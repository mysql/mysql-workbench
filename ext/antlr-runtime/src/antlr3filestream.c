/** \file
 * \brief The ANTLR3 C filestream is used when the source character stream
 * is a filesystem based input set and all the characters in the filestream
 * can be loaded at once into memory and away the lexer goes.
 *
 * A number of initializers are provided in order that various character
 * sets can be supported from input files. The ANTLR3 C runtime expects
 * to deal with UTF32 characters only (the reasons for this are to
 * do with the simplification of C code when using this form of Unicode 
 * encoding, though this is not a panacea. More information can be
 * found on this by consulting: 
 *   - http://www.unicode.org/versions/Unicode4.0.0/ch02.pdf#G11178
 * Where a well grounded discussion of the encoding formats available
 * may be found.
 *
 */

// [The "BSD licence"]
// Copyright (c) 2005-2009 Jim Idle, Temporal Wave LLC
// http://www.temporal-wave.com
// http://www.linkedin.com/in/jimidle
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include    <antlr3.h>

static  void                    setupInputStream            (pANTLR3_INPUT_STREAM input);
static  pANTLR3_INPUT_STREAM    antlr3CreateFileStream      (pANTLR3_UINT8 fileName);
static  pANTLR3_INPUT_STREAM    antlr3CreateStringStream    (pANTLR3_UINT8 data);

ANTLR3_API pANTLR3_INPUT_STREAM
antlr3FileStreamNew(pANTLR3_UINT8 fileName, ANTLR3_UINT32 encoding)
{
    pANTLR3_INPUT_STREAM input;

    // First order of business is to read the file into some buffer space
    // as just straight 8 bit bytes. Then we will work out the encoding and
    // byte order and adjust the API functions that are installed for the
    // default 8Bit stream accordingly.
    //
    input   = antlr3CreateFileStream(fileName);
    if  (input == NULL)
    {
        return NULL;
    }

    // We have the data in memory now so we can deal with it according to 
    // the encoding scheme we were given by the user.
    //
    input->encoding = encoding;

    // Now we need to work out the endian type and install any 
    // API functions that differ from 8Bit
    //
    setupInputStream(input);

    // Now we can set up the file name
    //	
    input->istream->streamName	= input->strFactory->newStr8(input->strFactory, fileName);
    input->fileName		= input->istream->streamName;

    return input;
}


ANTLR3_API pANTLR3_INPUT_STREAM
antlr3StringStreamNew(pANTLR3_UINT8 data, ANTLR3_UINT32 encoding, ANTLR3_UINT32 size, pANTLR3_UINT8 name)
{
    pANTLR3_INPUT_STREAM    input;

    // First order of business is to set up the stream and install the data pointer.
    // Then we will work out the encoding and byte order and adjust the API functions that are installed for the
    // default 8Bit stream accordingly.
    //
    input   = antlr3CreateStringStream(data);
    if  (input == NULL)
    {
        return NULL;
    }
    
    // Size (in bytes) of the given 'string'
    //
    input->sizeBuf		= size;

    // We have the data in memory now so we can deal with it according to 
    // the encoding scheme we were given by the user.
    //
    input->encoding = encoding;

    // Now we need to work out the endian type and install any 
    // API functions that differ from 8Bit
    //
    setupInputStream(input);

    // Now we can set up the file name
    //	
    input->istream->streamName	= input->strFactory->newStr8(input->strFactory, name);
    input->fileName		= input->istream->streamName;

    return input;
}


/// Determine endianess of the input stream and install the
/// API required for the encoding in that format.
///
static void 
setupInputStream(pANTLR3_INPUT_STREAM input)
{
    ANTLR3_BOOLEAN  isBigEndian;

    // Used to determine the endianness of the machine we are currently
    // running on.
    //
    ANTLR3_UINT16 bomTest = 0xFEFF;
    
    // What endianess is the machine we are running on? If the incoming
    // encoding endianess is the same as this machine's natural byte order
    // then we can use more efficient API calls.
    //
    if  (*((pANTLR3_UINT8)(&bomTest)) == 0xFE)
    {
        isBigEndian = ANTLR3_TRUE;
    }
    else
    {
        isBigEndian = ANTLR3_FALSE;
    }

    // What encoding did the user tell us {s}he thought it was? I am going
    // to get sick of the questions on antlr-interest, I know I am.
    //
    switch  (input->encoding)
    {
        case    ANTLR3_ENC_UTF8:

            // See if there is a BOM at the start of this UTF-8 sequence
            // and just eat it if there is. Windows .TXT files have this for instance
            // as it identifies UTF-8 even though it is of no consequence for byte order
            // as UTF-8 does not have a byte order.
            //
            if  (       (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar))      == 0xEF
                    &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+1))    == 0xBB
                    &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+2))    == 0xBF
                )
            {
                // The UTF8 BOM is present so skip it
                //
                input->nextChar = (void *)((pANTLR3_UINT8)input->nextChar + 3);
            }

            // Install the UTF8 input routines
            //
            antlr3UTF8SetupStream(input);
            break;

        case    ANTLR3_ENC_UTF16:

            // See if there is a BOM at the start of the input. If not then
            // we assume that the byte order is the natural order of this
            // machine (or it is really UCS2). If there is a BOM we determine if the encoding
            // is the same as the natural order of this machine.
            //
            if  (       (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar))      == 0xFE
                    &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+1))    == 0xFF
                )
            {
                // BOM Present, indicates Big Endian
                //
                input->nextChar = (void *)((pANTLR3_UINT8)input->nextChar + 2);

                antlr3UTF16SetupStream(input, isBigEndian, ANTLR3_TRUE);
            }
            else if  (      (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar))      == 0xFF
                        &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+1))    == 0xFE
                )
            {
                // BOM present, indicates Little Endian
                //
                input->nextChar = (void *)((pANTLR3_UINT8)input->nextChar + 2);

                antlr3UTF16SetupStream(input, isBigEndian, ANTLR3_FALSE);
            }
            else
            {
                // No BOM present, assume local computer byte order
                //
                antlr3UTF16SetupStream(input, isBigEndian, isBigEndian);
            }
            break;

        case    ANTLR3_ENC_UTF32:

            // See if there is a BOM at the start of the input. If not then
            // we assume that the byte order is the natural order of this
            // machine. If there is we determine if the encoding
            // is the same as the natural order of this machine.
            //
            if  (       (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar))      == 0x00
                    &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+1))    == 0x00
                    &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+2))    == 0xFE
                    &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+3))    == 0xFF
                )
            {
                // BOM Present, indicates Big Endian
                //
                input->nextChar = (void *)((pANTLR3_UINT8)input->nextChar + 4);

                antlr3UTF32SetupStream(input, isBigEndian, ANTLR3_TRUE);
            }
            else if  (      (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar))      == 0xFF
                        &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+1))    == 0xFE
                        &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+1))    == 0x00
                        &&  (ANTLR3_UINT8)(*((pANTLR3_UINT8)input->nextChar+1))    == 0x00
                )
            {
                // BOM present, indicates Little Endian
                //
                input->nextChar = (void *)((pANTLR3_UINT8)input->nextChar + 4);

                antlr3UTF32SetupStream(input, isBigEndian, ANTLR3_FALSE);
            }
            else
            {
                // No BOM present, assume local computer byte order
                //
                antlr3UTF32SetupStream(input, isBigEndian, isBigEndian);
            }
            break;

        case    ANTLR3_ENC_UTF16BE:

            // Encoding is definately Big Endian with no BOM
            //
            antlr3UTF16SetupStream(input, isBigEndian, ANTLR3_TRUE);
            break;

        case    ANTLR3_ENC_UTF16LE:

            // Encoding is definately Little Endian with no BOM
            //
            antlr3UTF16SetupStream(input, isBigEndian, ANTLR3_FALSE);
            break;

        case    ANTLR3_ENC_UTF32BE:

            // Encoding is definately Big Endian with no BOM
            //
            antlr3UTF32SetupStream(input, isBigEndian, ANTLR3_TRUE);
            break;

        case    ANTLR3_ENC_UTF32LE:

            // Encoding is definately Little Endian with no BOM
            //
            antlr3UTF32SetupStream(input, isBigEndian, ANTLR3_FALSE);
            break;

        case    ANTLR3_ENC_EBCDIC:

            // EBCDIC is basically the same as ASCII but with an on the
            // fly translation to ASCII
            //
            antlr3EBCDICSetupStream(input);
            break;

        case    ANTLR3_ENC_8BIT:
        default:

            // Standard 8bit/ASCII
            //
            antlr38BitSetupStream(input);
            break;
    }    
}

/** \brief Use the contents of an operating system file as the input
 *         for an input stream.
 *
 * \param fileName Name of operating system file to read.
 * \return
 *	- Pointer to new input stream context upon success
 *	- One of the ANTLR3_ERR_ defines on error.
 */
static pANTLR3_INPUT_STREAM
antlr3CreateFileStream(pANTLR3_UINT8 fileName)
{
	// Pointer to the input stream we are going to create
	//
	pANTLR3_INPUT_STREAM    input;
	ANTLR3_UINT32	    status;

	if	(fileName == NULL)
	{
		return NULL;
	}

	// Allocate memory for the input stream structure
	//
	input   = (pANTLR3_INPUT_STREAM)
		ANTLR3_CALLOC(1, sizeof(ANTLR3_INPUT_STREAM));

	if	(input == NULL)
	{
		return	NULL;
	}

	// Structure was allocated correctly, now we can read the file.
	//
	status  = antlr3read8Bit(input, fileName);

	// Call the common 8 bit input stream handler
	// initialization.
	//
	antlr3GenericSetupStream(input);

        // However if the file was not there or something then we
        // need to close. Have to wait until here as we cannot call
        // close until the API is installed of course.
        // 
	if	(status != ANTLR3_SUCCESS)
	{
		input->close(input);
		return	NULL;
	}

	return  input;
}

ANTLR3_API ANTLR3_UINT32
antlr3read8Bit(pANTLR3_INPUT_STREAM    input, pANTLR3_UINT8 fileName)
{
	ANTLR3_FDSC	    infile;
	ANTLR3_UINT32	    fSize;

	/* Open the OS file in read binary mode
	*/
	infile  = antlr3Fopen(fileName, "rb");

	/* Check that it was there
	*/
	if	(infile == NULL)
	{
		return	(ANTLR3_UINT32)ANTLR3_ERR_NOFILE;
	}

	/* It was there, so we can read the bytes now
	*/
	fSize   = antlr3Fsize(fileName);	/* Size of input file	*/

	/* Allocate buffer for this input set   
	*/
	input->data	    = ANTLR3_MALLOC((size_t)fSize);
	input->sizeBuf  = fSize;

	if	(input->data == NULL)
	{
		return	(ANTLR3_UINT32)ANTLR3_ERR_NOMEM;
	}

	input->isAllocated	= ANTLR3_TRUE;

	/* Now we read the file. Characters are not converted to
	* the internal ANTLR encoding until they are read from the buffer
	*/
	antlr3Fread(infile, fSize, input->data);

	/* And close the file handle
	*/
	antlr3Fclose(infile);

	return  ANTLR3_SUCCESS;
}

/** \brief Open an operating system file and return the descriptor
 * We just use the common open() and related functions here. 
 * Later we might find better ways on systems
 * such as Windows and OpenVMS for instance. But the idea is to read the 
 * while file at once anyway, so it may be irrelevant.
 */
ANTLR3_API ANTLR3_FDSC
antlr3Fopen(pANTLR3_UINT8 filename, const char * mode)
{
    return  (ANTLR3_FDSC)fopen((const char *)filename, mode);
}

/** \brief Close an operating system file and free any handles
 *  etc.
 */
ANTLR3_API void
antlr3Fclose(ANTLR3_FDSC fd)
{
    fclose(fd);
}
ANTLR3_API ANTLR3_UINT32
antlr3Fsize(pANTLR3_UINT8 fileName)
{   
    struct _stat	statbuf;

    _stat((const char *)fileName, &statbuf);

    return (ANTLR3_UINT32)statbuf.st_size;
}

ANTLR3_API ANTLR3_UINT32
antlr3Fread(ANTLR3_FDSC fdsc, ANTLR3_UINT32 count,  void * data)
{
    return  (ANTLR3_UINT32)fread(data, (size_t)count, 1, fdsc);
}


/** \brief Use the supplied 'string' as input to the stream
 *
 * \param data Pointer to the input data
 * \return
 *	- Pointer to new input stream context upon success
 *	- NULL defines on error.
 */
static pANTLR3_INPUT_STREAM
antlr3CreateStringStream(pANTLR3_UINT8 data)
{
	// Pointer to the input stream we are going to create
	//
	pANTLR3_INPUT_STREAM    input;

	if	(data == NULL)
	{
		return NULL;
	}

	// Allocate memory for the input stream structure
	//
	input   = (pANTLR3_INPUT_STREAM)
		ANTLR3_CALLOC(1, sizeof(ANTLR3_INPUT_STREAM));

	if	(input == NULL)
	{
		return	NULL;
	}

	// Structure was allocated correctly, now we can install the pointer
	//
        input->data             = data;
        input->isAllocated	= ANTLR3_FALSE;

	// Call the common 8 bit input stream handler
	// initialization.
	//
	antlr3GenericSetupStream(input);

        return  input;
}