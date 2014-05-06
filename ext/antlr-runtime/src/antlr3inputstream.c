/// \file
/// Base functions to initialize and manipulate any input stream
///

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

#include    <antlr3input.h>

// -----------------------------------
// Generic 8 bit input such as latin-1
//

// 8Bit INT Stream API
//
static	    void	    antlr38BitConsume		(pANTLR3_INT_STREAM is);
static	    ANTLR3_UCHAR    antlr38BitLA		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    ANTLR3_UCHAR    antlr38BitLA_ucase		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    ANTLR3_MARKER   antlr38BitIndex		(pANTLR3_INT_STREAM is);
static	    ANTLR3_MARKER   antlr38BitMark		(pANTLR3_INT_STREAM is);
static	    void	    antlr38BitRewind		(pANTLR3_INT_STREAM is, ANTLR3_MARKER mark);
static	    void	    antlr38BitRewindLast	(pANTLR3_INT_STREAM is);
static	    void	    antlr38BitRelease		(pANTLR3_INT_STREAM is, ANTLR3_MARKER mark);
static	    void	    antlr38BitSeek		(pANTLR3_INT_STREAM is, ANTLR3_MARKER seekPoint);
static	    pANTLR3_STRING  antlr38BitGetSourceName	(pANTLR3_INT_STREAM is);

// 8Bit Charstream API functions
//
static	    void	    antlr3InputClose		(pANTLR3_INPUT_STREAM input);
static	    void	    antlr3InputReset		(pANTLR3_INPUT_STREAM input);
static      void            antlr38BitReuse            (pANTLR3_INPUT_STREAM input, pANTLR3_UINT8 inString, ANTLR3_UINT32 size, pANTLR3_UINT8 name);
static	    void *	    antlr38BitLT		(pANTLR3_INPUT_STREAM input, ANTLR3_INT32 lt);
static	    ANTLR3_UINT32   antlr38BitSize		(pANTLR3_INPUT_STREAM input);
static	    pANTLR3_STRING  antlr38BitSubstr		(pANTLR3_INPUT_STREAM input, ANTLR3_MARKER start, ANTLR3_MARKER stop);
static	    ANTLR3_UINT32   antlr38BitGetLine		(pANTLR3_INPUT_STREAM input);
static	    void	  * antlr38BitGetLineBuf	(pANTLR3_INPUT_STREAM input);
static	    ANTLR3_UINT32   antlr38BitGetCharPosition	(pANTLR3_INPUT_STREAM input);
static	    void	    antlr38BitSetLine		(pANTLR3_INPUT_STREAM input, ANTLR3_UINT32 line);
static	    void	    antlr38BitSetCharPosition	(pANTLR3_INPUT_STREAM input, ANTLR3_UINT32 position);
static	    void	    antlr38BitSetNewLineChar	(pANTLR3_INPUT_STREAM input, ANTLR3_UINT32 newlineChar);
static	    void	    antlr38BitSetUcaseLA	(pANTLR3_INPUT_STREAM input, ANTLR3_BOOLEAN flag);

// -----------------------------------
// UTF16 (also covers UCS2)
//
// INT Stream API
//
static	    void	    antlr3UTF16Consume	        (pANTLR3_INT_STREAM is);
static	    ANTLR3_UCHAR    antlr3UTF16LA		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    void	    antlr3UTF16ConsumeLE        (pANTLR3_INT_STREAM is);
static	    ANTLR3_UCHAR    antlr3UTF16LALE		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    void	    antlr3UTF16ConsumeBE        (pANTLR3_INT_STREAM is);
static	    ANTLR3_UCHAR    antlr3UTF16LABE		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    ANTLR3_MARKER   antlr3UTF16Index		(pANTLR3_INT_STREAM is);
static	    void	    antlr3UTF16Seek		(pANTLR3_INT_STREAM is, ANTLR3_MARKER seekPoint);

// UTF16 Charstream API functions
//
static	    pANTLR3_STRING	antlr3UTF16Substr	(pANTLR3_INPUT_STREAM input, ANTLR3_MARKER start, ANTLR3_MARKER stop);

// -----------------------------------
// UTF32 (also covers UCS2)
//
// INT Stream API
//
static	    void	    antlr3UTF32Consume	        (pANTLR3_INT_STREAM is);
static	    ANTLR3_UCHAR    antlr3UTF32LA		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    ANTLR3_UCHAR    antlr3UTF32LALE		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    ANTLR3_UCHAR    antlr3UTF32LABE		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);
static	    ANTLR3_MARKER   antlr3UTF32Index		(pANTLR3_INT_STREAM is);
static	    void	    antlr3UTF32Seek		(pANTLR3_INT_STREAM is, ANTLR3_MARKER seekPoint);

// UTF16 Charstream API functions
//
static	    pANTLR3_STRING  antlr3UTF32Substr	        (pANTLR3_INPUT_STREAM input, ANTLR3_MARKER start, ANTLR3_MARKER stop);

// ------------------------------------
// UTF-8
//
static	    void	    antlr3UTF8Consume	        (pANTLR3_INT_STREAM is);
static	    ANTLR3_UCHAR    antlr3UTF8LA		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);

// ------------------------------------
// EBCDIC
//
static	    ANTLR3_UCHAR    antlr3EBCDICLA		(pANTLR3_INT_STREAM is, ANTLR3_INT32 la);

/// \brief Common function to setup function interface for an 8 bit input stream.
///
/// \param input Input stream context pointer
///
/// \remark
///   - Many of the 8 bit oriented file stream handling functions will be usable
///     by any or at least some, other input streams. Therefore it is perfectly acceptable
///     to call this function to install the 8Bit handler then override just those functions
///     that would not work for the particular input encoding, such as consume for instance.
/// 
void 
antlr38BitSetupStream	(pANTLR3_INPUT_STREAM input)
{
    // Build a string factory for this stream
    //
    input->strFactory	= antlr3StringFactoryNew(input->encoding);

    // Default stream API set up is for 8Bit, so we are done
    //
}

void
antlr3GenericSetupStream  (pANTLR3_INPUT_STREAM input)
{
    /* Install function pointers for an 8 bit input
     */

    /* Allocate stream interface
     */
    input->istream		= antlr3IntStreamNew();
    input->istream->type        = ANTLR3_CHARSTREAM;
    input->istream->super       = input;

    /* Intstream API
     */
    input->istream->consume	    = antlr38BitConsume;	    // Consume the next 8 bit character in the buffer			
    input->istream->_LA		    = antlr38BitLA;	            // Return the UTF32 character at offset n (1 based)			
    input->istream->index	    = antlr38BitIndex;	            // Current index (offset from first character			    
    input->istream->mark	    = antlr38BitMark;		    // Record the current lex state for later restore			
    input->istream->rewind	    = antlr38BitRewind;	            // How to rewind the input									
    input->istream->rewindLast	    = antlr38BitRewindLast;	    // How to rewind the input									
    input->istream->seek	    = antlr38BitSeek;		    // How to seek to a specific point in the stream		    
    input->istream->release	    = antlr38BitRelease;	    // Reset marks after mark n									
    input->istream->getSourceName   = antlr38BitGetSourceName;      // Return a string that names the input source

    /* Charstream API
     */
    input->close		    =  antlr3InputClose;	    // Close down the stream completely										
    input->free			    =  antlr3InputClose;	    // Synonym for free														
    input->reset		    =  antlr3InputReset;	    // Reset input to start	
    input->reuse                    =  antlr38BitReuse;             // Install a new input string and reset
    input->_LT			    =  antlr38BitLT;		    // Same as _LA for 8 bit file										
    input->size			    =  antlr38BitSize;		    // Return the size of the input buffer									
    input->substr		    =  antlr38BitSubstr;	    // Return a string from the input stream								
    input->getLine		    =  antlr38BitGetLine;	    // Return the current line number in the input stream					
    input->getLineBuf		    =  antlr38BitGetLineBuf;	    // Return a pointer to the start of the current line being consumed	    
    input->getCharPositionInLine    =  antlr38BitGetCharPosition;   // Return the offset into the current line of input						
    input->setLine		    =  antlr38BitSetLine;	    // Set the input stream line number (does not set buffer pointers)	    
    input->setCharPositionInLine    =  antlr38BitSetCharPosition;   // Set the offset in to the current line (does not set any pointers)   
    input->SetNewLineChar	    =  antlr38BitSetNewLineChar;    // Set the value of the newline trigger character						
    input->setUcaseLA		    =  antlr38BitSetUcaseLA;        // Changes the LA function to return upper case always

    input->charByteSize		    = 1;		// Size in bytes of characters in this stream.

    /* Initialize entries for tables etc
     */
    input->markers  = NULL;

    /* Set up the input stream brand new
     */
    input->reset(input);
    
    /* Install default line separator character (it can be replaced
     * by the grammar programmer later)
     */
    input->SetNewLineChar(input, (ANTLR3_UCHAR)'\n');
}

static pANTLR3_STRING
antlr38BitGetSourceName(pANTLR3_INT_STREAM is)
{
	return	is->streamName;
}

/** \brief Close down an input stream and free any memory allocated by it.
 *
 * \param input Input stream context pointer
 */
static void
antlr3InputClose(pANTLR3_INPUT_STREAM input)
{
    // Close any markers in the input stream
    //
    if	(input->markers != NULL)
    {
		input->markers->free(input->markers);
		input->markers = NULL;
    }

    // Close the string factory
    //
    if	(input->strFactory != NULL)
    {
		input->strFactory->close(input->strFactory);
    }

    // Free the input stream buffer if we allocated it
    //
    if	(input->isAllocated && input->data != NULL)
    {
		ANTLR3_FREE(input->data);
		input->data = NULL;
    }
    
    input->istream->free(input->istream);

    // Finally, free the space for the structure itself
    //
    ANTLR3_FREE(input);

    // Done
    //
}

static void		
antlr38BitSetUcaseLA		(pANTLR3_INPUT_STREAM input, ANTLR3_BOOLEAN flag)
{
	if	(flag)
	{
		// Return the upper case version of the characters
		//
		input->istream->_LA		    =  antlr38BitLA_ucase;
	}
	else
	{
		// Return the raw characters as they are in the buffer
		//
		input->istream->_LA		    =  antlr38BitLA;
	}
}


/** \brief Reset a re-startable input stream to the start
 *
 * \param input Input stream context pointer
 */
static void
antlr3InputReset(pANTLR3_INPUT_STREAM input)
{

    input->nextChar		= input->data;	/* Input at first character */
    input->line			= 1;		/* starts at line 1	    */
    input->charPositionInLine	= 0;
    input->currentLine		= input->data;
    input->markDepth		= 0;		/* Reset markers	    */
    
    /* Clear out up the markers table if it is there
     */
    if	(input->markers != NULL)
    {
        input->markers->clear(input->markers);
    }
    else
    {
        /* Install a new markers table
         */
        input->markers  = antlr3VectorNew(0);
    }
}

/** Install a new source code in to a working input stream so that the
 *  input stream can be reused.
 */
static void
antlr38BitReuse(pANTLR3_INPUT_STREAM input, pANTLR3_UINT8 inString, ANTLR3_UINT32 size, pANTLR3_UINT8 name)
{
    input->isAllocated	= ANTLR3_FALSE;
    input->data		= inString;
    input->sizeBuf	= size;
    
    // Now we can set up the file name. As we are reusing the stream, there may already
    // be a string that we can reuse for holding the filename.
    //
	if	(input->istream->streamName == NULL) 
	{
		input->istream->streamName	= input->strFactory->newStr(input->strFactory, name == NULL ? (pANTLR3_UINT8)"-memory-" : name);
		input->fileName		= input->istream->streamName;
	}
	else
	{
		input->istream->streamName->set(input->istream->streamName,  (name == NULL ? (const char *)"-memory-" : (const char *)name));
	}

    input->reset(input);
}

/** \brief Consume the next character in an 8 bit input stream
 *
 * \param input Input stream context pointer
 */
static void
antlr38BitConsume(pANTLR3_INT_STREAM is)
{
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {	
	/* Indicate one more character in this line
	 */
	input->charPositionInLine++;
	
	if  ((ANTLR3_UCHAR)(*((pANTLR3_UINT8)input->nextChar)) == input->newlineChar)
	{
	    /* Reset for start of a new line of input
	     */
	    input->line++;
	    input->charPositionInLine	= 0;
	    input->currentLine		= (void *)(((pANTLR3_UINT8)input->nextChar) + 1);
	}

	/* Increment to next character position
	 */
	input->nextChar = (void *)(((pANTLR3_UINT8)input->nextChar) + 1);
    }
}

/** \brief Return the input element assuming an 8 bit ascii input
 *
 * \param[in] input Input stream context pointer
 * \param[in] la 1 based offset of next input stream element
 *
 * \return Next input character in internal ANTLR3 encoding (UTF32)
 */
static ANTLR3_UCHAR 
antlr38BitLA(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
    pANTLR3_INPUT_STREAM input;
	
    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    if	(( ((pANTLR3_UINT8)input->nextChar) + la - 1) >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {
		return	ANTLR3_CHARSTREAM_EOF;
    }
    else
    {
		return	(ANTLR3_UCHAR)(*((pANTLR3_UINT8)input->nextChar + la - 1));
    }
}

/** \brief Return the input element assuming an 8 bit input and
 *         always return the UPPER CASE character.
 *		   Note that this is 8 bit and so we assume that the toupper
 *		   function will use the correct locale for 8 bits.
 *
 * \param[in] input Input stream context pointer
 * \param[in] la 1 based offset of next input stream element
 *
 * \return Next input character in internal ANTLR3 encoding (UTF32)
 */
static ANTLR3_UCHAR
antlr38BitLA_ucase	(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
    pANTLR3_INPUT_STREAM input;
	
    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    if	(( ((pANTLR3_UINT8)input->nextChar) + la - 1) >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {
		return	ANTLR3_CHARSTREAM_EOF;
    }
    else
    {
		return	(ANTLR3_UCHAR)toupper((*((pANTLR3_UINT8)input->nextChar + la - 1)));
    }
}


/** \brief Return the input element assuming an 8 bit ascii input
 *
 * \param[in] input Input stream context pointer
 * \param[in] lt 1 based offset of next input stream element
 *
 * \return Next input character in internal ANTLR3 encoding (UTF32)
 */
static void * 
antlr38BitLT(pANTLR3_INPUT_STREAM input, ANTLR3_INT32 lt)
{
    /* Casting is horrible but it means no warnings and LT should never be called
     * on a character stream anyway I think. If it is then, the void * will need to be 
     * cast back in a similar manner. Yuck! But this means that LT for Token streams and
     * tree streams is correct.
     */
    return (ANTLR3_FUNC_PTR(input->istream->_LA(input->istream, lt)));
}

/** \brief Calculate the current index in the output stream.
 * \param[in] input Input stream context pointer
 */
static ANTLR3_MARKER
antlr38BitIndex(pANTLR3_INT_STREAM is)
{
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    return  (ANTLR3_MARKER)(((pANTLR3_UINT8)input->nextChar));
}

/** \brief Return the size of the current input stream, as an 8Bit file
 *   which in this case is the total input. Other implementations may provide
 *   more sophisticated implementations to deal with non-recoverable streams 
 *   and so on.
 *
 * \param[in] input Input stream context pointer
 */
static	ANTLR3_UINT32 
antlr38BitSize(pANTLR3_INPUT_STREAM input)
{
    return  input->sizeBuf;
}

/** \brief Mark the current input point in an 8Bit 8 bit stream
 *  such as a file stream, where all the input is available in the
 *  buffer.
 *
 * \param[in] is Input stream context pointer
 */
static ANTLR3_MARKER
antlr38BitMark	(pANTLR3_INT_STREAM is)
{
    pANTLR3_LEX_STATE	    state;
    pANTLR3_INPUT_STREAM    input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    /* New mark point 
     */
    ++input->markDepth;

    /* See if we are revisiting a mark as we can just reuse the vector
     * entry if we are, otherwise, we need a new one
     */
    if	(input->markDepth > input->markers->count)
    {	
		state = (pANTLR3_LEX_STATE)ANTLR3_MALLOC(sizeof(ANTLR3_LEX_STATE));
		if (state == NULL)
		{
			// malloc failed
			--input->markDepth;
			return 0;
		}

		/* Add it to the table
		 */
		input->markers->add(input->markers, state, ANTLR3_FREE_FUNC);	/* No special structure, just free() on delete */
    }
    else
    {
		state	= (pANTLR3_LEX_STATE)input->markers->get(input->markers, input->markDepth - 1);

		/* Assume no errors for speed, it will just blow up if the table failed
		 * for some reasons, hence lots of unit tests on the tables ;-)
		 */
    }

    /* We have created or retrieved the state, so update it with the current
     * elements of the lexer state.
     */
    state->charPositionInLine	= input->charPositionInLine;
    state->currentLine		= input->currentLine;
    state->line			= input->line;
    state->nextChar		= input->nextChar;

    is->lastMarker  = input->markDepth;

    /* And that's it
     */
    return  input->markDepth;
}
/** \brief Rewind the lexer input to the state specified by the last produced mark.
 * 
 * \param[in] input Input stream context pointer
 *
 * \remark
 * Assumes 8 Bit input stream.
 */
static void
antlr38BitRewindLast	(pANTLR3_INT_STREAM is)
{
    is->rewind(is, is->lastMarker);
}

/** \brief Rewind the lexer input to the state specified by the supplied mark.
 * 
 * \param[in] input Input stream context pointer
 *
 * \remark
 * Assumes 8 Bit input stream.
 */
static void
antlr38BitRewind	(pANTLR3_INT_STREAM is, ANTLR3_MARKER mark)
{
    pANTLR3_LEX_STATE	state;
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) is->super);

    /* Perform any clean up of the marks
     */
    input->istream->release(input->istream, mark);

    /* Find the supplied mark state 
     */
    state   = (pANTLR3_LEX_STATE)input->markers->get(input->markers, (ANTLR3_UINT32)(mark - 1));
	if (state == NULL) { return; }

    /* Seek input pointer to the requested point (note we supply the void *pointer
     * to whatever is implementing the int stream to seek).
     */
    antlr38BitSeek(is, (ANTLR3_MARKER)(state->nextChar));

    /* Reset to the reset of the information in the mark
     */
    input->charPositionInLine	= state->charPositionInLine;
    input->currentLine		= state->currentLine;
    input->line			= state->line;
    input->nextChar		= state->nextChar;

    /* And we are done
     */
}

/** \brief Rewind the lexer input to the state specified by the supplied mark.
 * 
 * \param[in] input Input stream context pointer
 *
 * \remark
 * Assumes 8 Bit input stream.
 */
static void
antlr38BitRelease	(pANTLR3_INT_STREAM is, ANTLR3_MARKER mark)
{
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    /* We don't do much here in fact as we never free any higher marks in
     * the hashtable as we just resuse any memory allocated for them.
     */
    input->markDepth	= (ANTLR3_UINT32)(mark - 1);
}

/** \brief Rewind the lexer input to the state specified by the supplied mark.
 * 
 * \param[in] input Input stream context pointer
 *
 * \remark
 * Assumes 8 Bit input stream.
 */
static void
antlr38BitSeek	(pANTLR3_INT_STREAM is, ANTLR3_MARKER seekPoint)
{
	ANTLR3_INT32   count;
	pANTLR3_INPUT_STREAM input;

	input   = (pANTLR3_INPUT_STREAM)ANTLR3_FUNC_PTR(((pANTLR3_INPUT_STREAM) is->super));

	/* If the requested seek point is less than the current
	* input point, then we assume that we are resetting from a mark
	* and do not need to scan, but can just set to there.
	*/
	if	(seekPoint <= (ANTLR3_MARKER)(input->nextChar))
	{
		input->nextChar	= ((pANTLR3_UINT8) seekPoint);
	}
	else
	{
		count	= (ANTLR3_UINT32)(seekPoint - (ANTLR3_MARKER)(input->nextChar));

		while (count--)
		{
			is->consume(is);
		}
	}
}
/** Return a substring of the 8 bit input stream in
 *  newly allocated memory.
 *
 * \param input Input stream context pointer
 * \param start Offset in input stream where the string starts
 * \param stop  Offset in the input stream where the string ends.
 */
static pANTLR3_STRING
antlr38BitSubstr		(pANTLR3_INPUT_STREAM input, ANTLR3_MARKER start, ANTLR3_MARKER stop)
{
	return  input->strFactory->newPtr(input->strFactory, (pANTLR3_UINT8)start, (ANTLR3_UINT32)(stop - start + 1));
}

/** \brief Return the line number as understood by the 8 bit input stream.
 *
 * \param input Input stream context pointer
 * \return	Line number in input stream that we believe we are working on.
 */
static ANTLR3_UINT32   
antlr38BitGetLine		(pANTLR3_INPUT_STREAM input)
{
    return  input->line;
}

/** Return a pointer into the input stream that points at the start
 *  of the current input line as triggered by the end of line character installed
 *  for the stream ('\n' unless told differently).
 *
 * \param[in] input 
 */
static void	  * 
antlr38BitGetLineBuf	(pANTLR3_INPUT_STREAM input)
{
    return  input->currentLine;
}

/** Return the current offset in to the current line in the input stream.
 *
 * \param input Input stream context pointer
 * \return      Current line offset
 */
static ANTLR3_UINT32
antlr38BitGetCharPosition	(pANTLR3_INPUT_STREAM input)
{
    return  input->charPositionInLine;
}

/** Set the current line number as understood by the input stream.
 *
 * \param input Input stream context pointer
 * \param line  Line number to tell the input stream we are on
 *
 * \remark
 *  This function does not change any pointers, it just allows the programmer to set the
 *  line number according to some external criterion, such as finding a lexed directive
 *  like: #nnn "file.c" for instance, such that error reporting and so on in is in sync
 *  with some original source format.
 */
static void
antlr38BitSetLine		(pANTLR3_INPUT_STREAM input, ANTLR3_UINT32 line)
{
    input->line	= line;
}

/** Set the current offset in the current line to be a particular setting.
 *
 * \param[in] input    Input stream context pointer
 * \param[in] position New setting for current offset.
 *
 * \remark
 * This does not set the actual pointers in the input stream, it is purely for reporting
 * purposes and so on as per antlr38BitSetLine();
 */
static void
antlr38BitSetCharPosition	(pANTLR3_INPUT_STREAM input, ANTLR3_UINT32 position)
{
    input->charPositionInLine = position;
}

/** Set the newline trigger character in the input stream to the supplied parameter.
 *
 * \param[in] input	    Input stream context pointer
 * \param[in] newlineChar   Character to set to be the newline trigger.
 *
 * \remark
 *  - The supplied newLineChar is in UTF32 encoding (which means ASCII and latin1 etc
 *    are the same encodings), but the input stream catered to by this function is 8 bit
 *    only, so it is up to the programmer to ensure that the character supplied is valid.
 */
static void 
antlr38BitSetNewLineChar	(pANTLR3_INPUT_STREAM input, ANTLR3_UINT32 newlineChar)
{
    input->newlineChar	= newlineChar;
}


/// \brief Common function to setup function interface for a UTF16 or UCS2 input stream.
///
/// \param input Input stream context pointer
///
/// \remark
///  - Strictly speaking, there is no such thing as a UCS2 input stream as the term
///    tends to confuse the notions of character encoding, unicode and so on. UCS2 is
///    essentially UTF16 without any surrogates and so the standard UTF16
///    input stream is able to handle it without any special code.
///
void 
antlr3UTF16SetupStream	(pANTLR3_INPUT_STREAM input, ANTLR3_BOOLEAN machineBigEndian, ANTLR3_BOOLEAN inputBigEndian)
{
    // Build a string factory for this stream. This is a UTF16 string factory which is a standard
    // part of the ANTLR3 string. The string factory is then passed through the whole chain 
    // of lexer->parser->tree->treeparser and so on.
    //
    input->strFactory	= antlr3StringFactoryNew(input->encoding);

    // Generic API that does not care about endianess.
    //
    input->istream->index	    =  antlr3UTF16Index;            // Calculate current index in input stream, UTF16 based
    input->substr		    =  antlr3UTF16Substr;	    // Return a string from the input stream
    input->istream->seek	    =  antlr3UTF16Seek;		    // How to seek to a specific point in the stream

    // We must install different UTF16 routines according to whether the input
    // is the same endianess as the machine we are executing upon or not. If it is not
    // then we must install methods that can convert the endianess on the fly as they go
    //

    switch (machineBigEndian)
    {
        case    ANTLR3_TRUE:

            // Machine is Big Endian, if the input is also then install the 
            // methods that do not access input by bytes and reverse them.
            // Otherwise install endian aware methods.
            //
            if  (inputBigEndian == ANTLR3_TRUE) 
            {
                // Input is machine compatible
                //
                input->istream->consume	    =  antlr3UTF16Consume;	    // Consume the next UTF16 character in the buffer
                input->istream->_LA         =  antlr3UTF16LA;		    // Return the UTF32 character at offset n (1 based)    
            }
            else
            {
                // Need to use methods that know that the input is little endian
                //
                input->istream->consume	    =  antlr3UTF16ConsumeLE;	    // Consume the next UTF16 character in the buffer
                input->istream->_LA         =  antlr3UTF16LALE;		    // Return the UTF32 character at offset n (1 based) 
            }
            break;

        case    ANTLR3_FALSE:

            // Machine is Little Endian, if the input is also then install the 
            // methods that do not access input by bytes and reverse them.
            // Otherwise install endian aware methods.
            //
            if  (inputBigEndian == ANTLR3_FALSE) 
            {
                // Input is machine compatible
                //
                input->istream->consume	    =  antlr3UTF16Consume;	    // Consume the next UTF16 character in the buffer
                input->istream->_LA         =  antlr3UTF16LA;		    // Return the UTF32 character at offset n (1 based)    
            }
            else
            {
                // Need to use methods that know that the input is Big Endian
                //
                input->istream->consume	    =  antlr3UTF16ConsumeBE;	    // Consume the next UTF16 character in the buffer
                input->istream->_LA         =  antlr3UTF16LABE;		    // Return the UTF32 character at offset n (1 based) 
            }
            break;
    }

        
    input->charByteSize		    = 2;			    // Size in bytes of characters in this stream.

}

/// \brief Consume the next character in a UTF16 input stream
///
/// \param input Input stream context pointer
///
static void
antlr3UTF16Consume(pANTLR3_INT_STREAM is)
{
	pANTLR3_INPUT_STREAM input;
        UTF32   ch;
        UTF32   ch2;

	input   = ((pANTLR3_INPUT_STREAM) (is->super));

        // Buffer size is always in bytes
        //
	if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
	{	
		// Indicate one more character in this line
		//
		input->charPositionInLine++;

		if  ((ANTLR3_UCHAR)(*((pANTLR3_UINT16)input->nextChar)) == input->newlineChar)
		{
			// Reset for start of a new line of input
			//
			input->line++;
			input->charPositionInLine	= 0;
			input->currentLine		= (void *)(((pANTLR3_UINT16)input->nextChar) + 1);
		}

		// Increment to next character position, accounting for any surrogates
		//
                // Next char in natural machine byte order
                //
                ch  = *((UTF16*)input->nextChar);

                // We consumed one 16 bit character
                //
		input->nextChar = (void *)(((pANTLR3_UINT16)input->nextChar) + 1);

                // If we have a surrogate pair then we need to consume
                // a following valid LO surrogate.
                //
                if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {

                    // If the 16 bits following the high surrogate are in the source buffer...
                    //
                    if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                    {
                        // Next character is in natural machine byte order
                        //
                        ch2 = *((UTF16*)input->nextChar);

                        // If it's a valid low surrogate, consume it
                        //
                        if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                        {
                            // We consumed one 16 bit character
                            //
		            input->nextChar = (void *)(((pANTLR3_UINT16)input->nextChar) + 1);
                        }
                        // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                        // it.
                        //
                    } 
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it because the buffer ended
                    //
                } 
                // Note that we did not check for an invalid low surrogate here, or that fact that the
                // lo surrogate was missing. We just picked out one 16 bit character unless the character
                // was a valid hi surrogate, in whcih case we consumed two 16 bit characters.
                //
	}
}

/// \brief Return the input element assuming an 8 bit ascii input
///
/// \param[in] input Input stream context pointer
/// \param[in] la 1 based offset of next input stream element
///
/// \return Next input character in internal ANTLR3 encoding (UTF32)
///
static ANTLR3_UCHAR 
antlr3UTF16LA(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
	pANTLR3_INPUT_STREAM input;
        UTF32   ch;
        UTF32   ch2;
        UTF16   * nextChar;

        // Find the input interface and where we are currently pointing to
        // in the input stream
        //
	input       = ((pANTLR3_INPUT_STREAM) (is->super));
        nextChar    = (UTF16*)input->nextChar;

        // If a positive offset then advance forward, else retreat
        //
        if  (la >= 0)
        {
            while   (--la > 0 && (pANTLR3_UINT8)nextChar < ((pANTLR3_UINT8)input->data) + input->sizeBuf )
            {
                // Advance our copy of the input pointer
                //
                // Next char in natural machine byte order
                //
                ch  = *nextChar++;

                // If we have a surrogate pair then we need to consume
                // a following valid LO surrogate.
                //
                if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
                {
                    // If the 16 bits following the high surrogate are in the source buffer...
                    //
                    if	((pANTLR3_UINT8)(nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                    {
                        // Next character is in natural machine byte order
                        //
                        ch2 = *nextChar;

                        // If it's a valid low surrogate, consume it
                        //
                        if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                        {
                            // We consumed one 16 bit character
                            //
		            nextChar++;
                        }
                        // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                        // it.
                        //
                    } 
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it because the buffer ended
                    //
                }
                // Note that we did not check for an invalid low surrogate here, or that fact that the
                // lo surrogate was missing. We just picked out one 16 bit character unless the character
                // was a valid hi surrogate, in whcih case we consumed two 16 bit characters.
                //
            }
        }
        else
        {
            // We need to go backwards from our input point
            //
            while   (la++ < 0 && (pANTLR3_UINT8)nextChar > (pANTLR3_UINT8)input->data )
            {
                // Get the previous 16 bit character
                //
                ch = *--nextChar;

                // If we found a low surrogate then go back one more character if
                // the hi surrogate is there
                //
                if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) 
                {
                    ch2 = *(nextChar-1);
                    if (ch2 >= UNI_SUR_HIGH_START && ch2 <= UNI_SUR_HIGH_END) 
                    {
                        // Yes, there is a high surrogate to match it so decrement one more and point to that
                        //
                        nextChar--;
                    }
                }
            }
        }

        // Our local copy of nextChar is now pointing to either the correct character or end of file
        //
        // Input buffer size is always in bytes
        //
	if	( (pANTLR3_UINT8)nextChar >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
	{
		return	ANTLR3_CHARSTREAM_EOF;
	}
	else
	{
            // Pick up the next 16 character (native machine byte order)
            //
            ch = *nextChar++;

            // If we have a surrogate pair then we need to consume
            // a following valid LO surrogate.
            //
            if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
            {
                // If the 16 bits following the high surrogate are in the source buffer...
                //
                if	((pANTLR3_UINT8)(nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                {
                    // Next character is in natural machine byte order
                    //
                    ch2 = *nextChar;

                    // If it's a valid low surrogate, consume it
                    //
                    if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                    {
                        // Construct the UTF32 code point
                        //
                        ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
			    + (ch2 - UNI_SUR_LOW_START) + halfBase;
                    }
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it.
                    //
                } 
                // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                // it because the buffer ended
                //
            }
        }
        return ch;
}


/// \brief Calculate the current index in the output stream.
/// \param[in] input Input stream context pointer
///
static ANTLR3_MARKER 
antlr3UTF16Index(pANTLR3_INT_STREAM is)
{
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    return  (ANTLR3_MARKER)(input->nextChar);
}

/// \brief Rewind the lexer input to the state specified by the supplied mark.
///
/// \param[in] input Input stream context pointer
///
/// \remark
/// Assumes UTF16 input stream.
///
static void
antlr3UTF16Seek	(pANTLR3_INT_STREAM is, ANTLR3_MARKER seekPoint)
{
	pANTLR3_INPUT_STREAM input;

	input   = ((pANTLR3_INPUT_STREAM) is->super);

	// If the requested seek point is less than the current
	// input point, then we assume that we are resetting from a mark
	// and do not need to scan, but can just set to there as rewind will
        // reset line numbers and so on.
	//
	if	(seekPoint <= (ANTLR3_MARKER)(input->nextChar))
	{
		input->nextChar	= (void *)seekPoint;
	}
	else
	{
            // Call consume until we reach the asked for seek point or EOF
            //
            while (is->_LA(is, 1) != ANTLR3_CHARSTREAM_EOF && seekPoint < (ANTLR3_MARKER)input->nextChar)
	    {
		is->consume(is);
	    }
	}
}
/// \brief Return a substring of the UTF16 input stream in
///  newly allocated memory.
///
/// \param input Input stream context pointer
/// \param start Offset in input stream where the string starts
/// \param stop  Offset in the input stream where the string ends.
///
static pANTLR3_STRING
antlr3UTF16Substr		(pANTLR3_INPUT_STREAM input, ANTLR3_MARKER start, ANTLR3_MARKER stop)
{
    return  input->strFactory->newPtr(input->strFactory, (pANTLR3_UINT8)start, ((ANTLR3_UINT32_CAST(stop - start))/2) + 1);
}

/// \brief Consume the next character in a UTF16 input stream when the input is Little Endian and the machine is not
/// Note that the UTF16 routines do not do any substantial verification of the input stream as for performance
/// sake, we assume it is validly encoded. So if a low surrogate is found at the curent input position then we
/// just consume it. Surrogate pairs should be seen as Hi, Lo. So if we have a Lo first, then the input stream
/// is fubar but we just ignore that.
///
/// \param input Input stream context pointer
///
static void
antlr3UTF16ConsumeLE(pANTLR3_INT_STREAM is)
{
	pANTLR3_INPUT_STREAM input;
        UTF32   ch;
        UTF32   ch2;

	input   = ((pANTLR3_INPUT_STREAM) (is->super));

        // Buffer size is always in bytes
        //
	if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
	{	
		// Indicate one more character in this line
		//
		input->charPositionInLine++;

		if  ((ANTLR3_UCHAR)(*((pANTLR3_UINT16)input->nextChar)) == input->newlineChar)
		{
			// Reset for start of a new line of input
			//
			input->line++;
			input->charPositionInLine	= 0;
			input->currentLine		= (void *)(((pANTLR3_UINT16)input->nextChar) + 1);
		}

		// Increment to next character position, accounting for any surrogates
		//
                // Next char in litle endian form
                //
                ch  = *((pANTLR3_UINT8)input->nextChar) + (*((pANTLR3_UINT8)input->nextChar + 1) <<8);

                // We consumed one 16 bit character
                //
		input->nextChar = (void *)(((pANTLR3_UINT16)input->nextChar) + 1);

                // If we have a surrogate pair then we need to consume
                // a following valid LO surrogate.
                //
                if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {

                    // If the 16 bits following the high surrogate are in the source buffer...
                    //
                    if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                    {
                        ch2 = *((pANTLR3_UINT8)input->nextChar) + (*((pANTLR3_UINT8)input->nextChar + 1) <<8);

                        // If it's a valid low surrogate, consume it
                        //
                        if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                        {
                            // We consumed one 16 bit character
                            //
		            input->nextChar = (void *)(((pANTLR3_UINT16)input->nextChar) + 1);
                        }
                        // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                        // it.
                        //
                    } 
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it because the buffer ended
                    //
                } 
                // Note that we did not check for an invalid low surrogate here, or that fact that the
                // lo surrogate was missing. We just picked out one 16 bit character unless the character
                // was a valid hi surrogate, in whcih case we consumed two 16 bit characters.
                //
	}
}

/// \brief Return the input element assuming a UTF16 input when the input is Little Endian and the machine is not
///
/// \param[in] input Input stream context pointer
/// \param[in] la 1 based offset of next input stream element
///
/// \return Next input character in internal ANTLR3 encoding (UTF32)
///
static ANTLR3_UCHAR 
antlr3UTF16LALE(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
	pANTLR3_INPUT_STREAM input;
        UTF32           ch;
        UTF32           ch2;
        pANTLR3_UCHAR   nextChar;

        // Find the input interface and where we are currently pointing to
        // in the input stream
        //
	input       = ((pANTLR3_INPUT_STREAM) (is->super));
        nextChar    = (pANTLR3_UCHAR)input->nextChar;

        // If a positive offset then advance forward, else retreat
        //
        if  (la >= 0)
        {
            while   (--la > 0 && (pANTLR3_UINT8)nextChar < ((pANTLR3_UINT8)input->data) + input->sizeBuf )
            {
                // Advance our copy of the input pointer
                //
                // Next char in Little Endian byte order
                //
                ch  = (*nextChar) + (*(nextChar+1) << 8);
                nextChar += 2;

                // If we have a surrogate pair then we need to consume
                // a following valid LO surrogate.
                //
                if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
                {
                    // If the 16 bits following the high surrogate are in the source buffer...
                    //
                    if	((pANTLR3_UINT8)(nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                    {
                        // Next character is in little endian byte order
                        //
                        ch2 = (*nextChar) + (*(nextChar+1) << 8);

                        // If it's a valid low surrogate, consume it
                        //
                        if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                        {
                            // We consumed one 16 bit character
                            //
		            nextChar += 2;
                        }
                        // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                        // it.
                        //
                    } 
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it because the buffer ended
                    //
                }
                // Note that we did not check for an invalid low surrogate here, or that fact that the
                // lo surrogate was missing. We just picked out one 16 bit character unless the character
                // was a valid hi surrogate, in whcih case we consumed two 16 bit characters.
                //
            }
        }
        else
        {
            // We need to go backwards from our input point
            //
            while   (la++ < 0 && (pANTLR3_UINT8)nextChar > (pANTLR3_UINT8)input->data )
            {
                // Get the previous 16 bit character
                //
                ch = (*nextChar - 2) + ((*nextChar -1) << 8);
                nextChar -= 2;

                // If we found a low surrogate then go back one more character if
                // the hi surrogate is there
                //
                if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) 
                {
                    ch2 = (*nextChar - 2) + ((*nextChar -1) << 8);
                    if (ch2 >= UNI_SUR_HIGH_START && ch2 <= UNI_SUR_HIGH_END) 
                    {
                        // Yes, there is a high surrogate to match it so decrement one more and point to that
                        //
                        nextChar -=2;
                    }
                }
            }
        }

        // Our local copy of nextChar is now pointing to either the correct character or end of file
        //
        // Input buffer size is always in bytes
        //
	if	( (pANTLR3_UINT8)nextChar >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
	{
		return	ANTLR3_CHARSTREAM_EOF;
	}
	else
	{
            // Pick up the next 16 character (little endian byte order)
            //
            ch = (*nextChar) + (*(nextChar+1) << 8);
            nextChar += 2;

            // If we have a surrogate pair then we need to consume
            // a following valid LO surrogate.
            //
            if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
            {
                // If the 16 bits following the high surrogate are in the source buffer...
                //
                if	((pANTLR3_UINT8)(nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                {
                    // Next character is in little endian byte order
                    //
                    ch2 = (*nextChar) + (*(nextChar+1) << 8);

                    // If it's a valid low surrogate, consume it
                    //
                    if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                    {
                        // Construct the UTF32 code point
                        //
                        ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
			    + (ch2 - UNI_SUR_LOW_START) + halfBase;
                    }
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it.
                    //
                } 
                // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                // it because the buffer ended
                //
            }
        }
        return ch;
}

/// \brief Consume the next character in a UTF16 input stream when the input is Big Endian and the machine is not
///
/// \param input Input stream context pointer
///
static void
antlr3UTF16ConsumeBE(pANTLR3_INT_STREAM is)
{
	pANTLR3_INPUT_STREAM input;
        UTF32   ch;
        UTF32   ch2;

	input   = ((pANTLR3_INPUT_STREAM) (is->super));

        // Buffer size is always in bytes
        //
	if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
	{	
		// Indicate one more character in this line
		//
		input->charPositionInLine++;

		if  ((ANTLR3_UCHAR)(*((pANTLR3_UINT16)input->nextChar)) == input->newlineChar)
		{
			// Reset for start of a new line of input
			//
			input->line++;
			input->charPositionInLine	= 0;
			input->currentLine		= (void *)(((pANTLR3_UINT16)input->nextChar) + 1);
		}

		// Increment to next character position, accounting for any surrogates
		//
                // Next char in big endian form
                //
                ch  = *((pANTLR3_UINT8)input->nextChar + 1) + (*((pANTLR3_UINT8)input->nextChar ) <<8);

                // We consumed one 16 bit character
                //
		input->nextChar = (void *)(((pANTLR3_UINT16)input->nextChar) + 1);

                // If we have a surrogate pair then we need to consume
                // a following valid LO surrogate.
                //
                if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {

                    // If the 16 bits following the high surrogate are in the source buffer...
                    //
                    if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                    {
                        // Big endian
                        //
                        ch2 = *((pANTLR3_UINT8)input->nextChar + 1) + (*((pANTLR3_UINT8)input->nextChar ) <<8);

                        // If it's a valid low surrogate, consume it
                        //
                        if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                        {
                            // We consumed one 16 bit character
                            //
		            input->nextChar = (void *)(((pANTLR3_UINT16)input->nextChar) + 1);
                        }
                        // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                        // it.
                        //
                    } 
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it because the buffer ended
                    //
                } 
                // Note that we did not check for an invalid low surrogate here, or that fact that the
                // lo surrogate was missing. We just picked out one 16 bit character unless the character
                // was a valid hi surrogate, in whcih case we consumed two 16 bit characters.
                //
	}
}

/// \brief Return the input element assuming a UTF16 input when the input is Little Endian and the machine is not
///
/// \param[in] input Input stream context pointer
/// \param[in] la 1 based offset of next input stream element
///
/// \return Next input character in internal ANTLR3 encoding (UTF32)
///
static ANTLR3_UCHAR 
antlr3UTF16LABE(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
	pANTLR3_INPUT_STREAM input;
        UTF32           ch;
        UTF32           ch2;
        pANTLR3_UCHAR   nextChar;

        // Find the input interface and where we are currently pointing to
        // in the input stream
        //
	input       = ((pANTLR3_INPUT_STREAM) (is->super));
        nextChar    = (pANTLR3_UCHAR)input->nextChar;

        // If a positive offset then advance forward, else retreat
        //
        if  (la >= 0)
        {
            while   (--la > 0 && (pANTLR3_UINT8)nextChar < ((pANTLR3_UINT8)input->data) + input->sizeBuf )
            {
                // Advance our copy of the input pointer
                //
                // Next char in Big Endian byte order
                //
                ch  = ((*nextChar) << 8) + *(nextChar+1);
                nextChar += 2;

                // If we have a surrogate pair then we need to consume
                // a following valid LO surrogate.
                //
                if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
                {
                    // If the 16 bits following the high surrogate are in the source buffer...
                    //
                    if	((pANTLR3_UINT8)(nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                    {
                        // Next character is in big endian byte order
                        //
                        ch2 = ((*nextChar) << 8) + *(nextChar+1);

                        // If it's a valid low surrogate, consume it
                        //
                        if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                        {
                            // We consumed one 16 bit character
                            //
		            nextChar += 2;
                        }
                        // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                        // it.
                        //
                    } 
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it because the buffer ended
                    //
                }
                // Note that we did not check for an invalid low surrogate here, or that fact that the
                // lo surrogate was missing. We just picked out one 16 bit character unless the character
                // was a valid hi surrogate, in whcih case we consumed two 16 bit characters.
                //
            }
        }
        else
        {
            // We need to go backwards from our input point
            //
            while   (la++ < 0 && (pANTLR3_UINT8)nextChar > (pANTLR3_UINT8)input->data )
            {
                // Get the previous 16 bit character
                //
                ch = ((*nextChar - 2) << 8) + (*nextChar -1);
                nextChar -= 2;

                // If we found a low surrogate then go back one more character if
                // the hi surrogate is there
                //
                if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) 
                {
                    ch2 = ((*nextChar - 2) << 8) + (*nextChar -1);
                    if (ch2 >= UNI_SUR_HIGH_START && ch2 <= UNI_SUR_HIGH_END) 
                    {
                        // Yes, there is a high surrogate to match it so decrement one more and point to that
                        //
                        nextChar -=2;
                    }
                }
            }
        }

        // Our local copy of nextChar is now pointing to either the correct character or end of file
        //
        // Input buffer size is always in bytes
        //
	if	( (pANTLR3_UINT8)nextChar >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
	{
		return	ANTLR3_CHARSTREAM_EOF;
	}
	else
	{
            // Pick up the next 16 character (big endian byte order)
            //
            ch = ((*nextChar) << 8) + *(nextChar+1);
            nextChar += 2;

            // If we have a surrogate pair then we need to consume
            // a following valid LO surrogate.
            //
            if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
            {
                // If the 16 bits following the high surrogate are in the source buffer...
                //
                if	((pANTLR3_UINT8)(nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                {
                    // Next character is in big endian byte order
                    //
                    ch2 = ((*nextChar) << 8) + *(nextChar+1);

                    // If it's a valid low surrogate, consume it
                    //
                    if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
                    {
                        // Construct the UTF32 code point
                        //
                        ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
			    + (ch2 - UNI_SUR_LOW_START) + halfBase;
                    }
                    // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                    // it.
                    //
                } 
                // Note that we ignore a valid hi surrogate that has no lo surrogate to go with
                // it because the buffer ended
                //
            }
        }
        return ch;
}

/// \brief Common function to setup function interface for a UTF3 input stream.
///
/// \param input Input stream context pointer
///
void 
antlr3UTF32SetupStream	(pANTLR3_INPUT_STREAM input, ANTLR3_BOOLEAN machineBigEndian, ANTLR3_BOOLEAN inputBigEndian)
{
    // Build a string factory for this stream. This is a UTF32 string factory which is a standard
    // part of the ANTLR3 string. The string factory is then passed through the whole chain of lexer->parser->tree->treeparser
    // and so on.
    //
    input->strFactory	= antlr3StringFactoryNew(input->encoding);

    // Generic API that does not care about endianess.
    //
    input->istream->index	    =  antlr3UTF32Index;            // Calculate current index in input stream, UTF16 based
    input->substr		    =  antlr3UTF32Substr;	    // Return a string from the input stream
    input->istream->seek	    =  antlr3UTF32Seek;		    // How to seek to a specific point in the stream
    input->istream->consume	    =  antlr3UTF32Consume;	    // Consume the next UTF32 character in the buffer

    // We must install different UTF32 LA routines according to whether the input
    // is the same endianess as the machine we are executing upon or not. If it is not
    // then we must install methods that can convert the endianess on the fly as they go
    //
    switch (machineBigEndian)
    {
        case    ANTLR3_TRUE:

            // Machine is Big Endian, if the input is also then install the 
            // methods that do not access input by bytes and reverse them.
            // Otherwise install endian aware methods.
            //
            if  (inputBigEndian == ANTLR3_TRUE) 
            {
                // Input is machine compatible
                //
                input->istream->_LA         =  antlr3UTF32LA;		    // Return the UTF32 character at offset n (1 based)    
            }
            else
            {
                // Need to use methods that know that the input is little endian
                //
                input->istream->_LA         =  antlr3UTF32LALE;		    // Return the UTF32 character at offset n (1 based) 
            }
            break;

        case    ANTLR3_FALSE:

            // Machine is Little Endian, if the input is also then install the 
            // methods that do not access input by bytes and reverse them.
            // Otherwise install endian aware methods.
            //
            if  (inputBigEndian == ANTLR3_FALSE) 
            {
                // Input is machine compatible
                //
                input->istream->_LA         =  antlr3UTF32LA;		    // Return the UTF32 character at offset n (1 based)    
            }
            else
            {
                // Need to use methods that know that the input is Big Endian
                //
                input->istream->_LA         =  antlr3UTF32LABE;		    // Return the UTF32 character at offset n (1 based) 
            }
            break;
    }

    input->charByteSize		    = 4;			    // Size in bytes of characters in this stream.
}

/** \brief Consume the next character in a UTF32 input stream
 *
 * \param input Input stream context pointer
 */
static void
antlr3UTF32Consume(pANTLR3_INT_STREAM is)
{
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    // SizeBuf is always in bytes
    //
    if	((pANTLR3_UINT8)(input->nextChar) < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {	
	/* Indicate one more character in this line
	 */
	input->charPositionInLine++;
	
	if  ((ANTLR3_UCHAR)(*((pANTLR3_UINT32)input->nextChar)) == input->newlineChar)
	{
	    /* Reset for start of a new line of input
	     */
	    input->line++;
	    input->charPositionInLine	= 0;
	    input->currentLine		= (void *)(((pANTLR3_UINT32)input->nextChar) + 1);
	}

	/* Increment to next character position
	 */
	input->nextChar = (void *)(((pANTLR3_UINT32)input->nextChar) + 1);
    }
}

/// \brief Calculate the current index in the output stream.
/// \param[in] input Input stream context pointer
///
static ANTLR3_MARKER 
antlr3UTF32Index(pANTLR3_INT_STREAM is)
{
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    return  (ANTLR3_MARKER)(input->nextChar);
}

/// \brief Return a substring of the UTF16 input stream in
///  newly allocated memory.
///
/// \param input Input stream context pointer
/// \param start Offset in input stream where the string starts
/// \param stop  Offset in the input stream where the string ends.
///
static pANTLR3_STRING
antlr3UTF32Substr		(pANTLR3_INPUT_STREAM input, ANTLR3_MARKER start, ANTLR3_MARKER stop)
{
    return  input->strFactory->newPtr(input->strFactory, (pANTLR3_UINT8)start, ((ANTLR3_UINT32_CAST(stop - start))/4) + 1);
}

/// \brief Rewind the lexer input to the state specified by the supplied mark.
///
/// \param[in] input Input stream context pointer
///
/// \remark
/// Assumes UTF32 input stream.
///
static void
antlr3UTF32Seek	(pANTLR3_INT_STREAM is, ANTLR3_MARKER seekPoint)
{
	pANTLR3_INPUT_STREAM input;

	input   = ((pANTLR3_INPUT_STREAM) is->super);

	// If the requested seek point is less than the current
	// input point, then we assume that we are resetting from a mark
	// and do not need to scan, but can just set to there as rewind will
        // reset line numbers and so on.
	//
	if	(seekPoint <= (ANTLR3_MARKER)(input->nextChar))
	{
		input->nextChar	= (void *)seekPoint;
	}
	else
	{
            // Call consume until we reach the asked for seek point or EOF
            //
            while (is->_LA(is, 1) != ANTLR3_CHARSTREAM_EOF && seekPoint < (ANTLR3_MARKER)input->nextChar)
	    {
		is->consume(is);
	    }
	}
}

/** \brief Return the input element assuming a UTF32 input in natural machine byte order
 *
 * \param[in] input Input stream context pointer
 * \param[in] la 1 based offset of next input stream element
 *
 * \return Next input character in internal ANTLR3 encoding (UTF32)
 */
static ANTLR3_UCHAR 
antlr3UTF32LA(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
    pANTLR3_INPUT_STREAM input;
	
    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    if	(( ((pANTLR3_UINT8)input->nextChar) + la - 1) >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {
		return	ANTLR3_CHARSTREAM_EOF;
    }
    else
    {
		return	(ANTLR3_UCHAR)(*((pANTLR3_UINT32)input->nextChar + la - 1));
    }
}

/** \brief Return the input element assuming a UTF32 input in little endian byte order
 *
 * \param[in] input Input stream context pointer
 * \param[in] la 1 based offset of next input stream element
 *
 * \return Next input character in internal ANTLR3 encoding (UTF32)
 */
static ANTLR3_UCHAR 
antlr3UTF32LALE(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
    pANTLR3_INPUT_STREAM input;
	
    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    if	(( ((pANTLR3_UINT8)input->nextChar) + la - 1) >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {
		return	ANTLR3_CHARSTREAM_EOF;
    }
    else
    {
        ANTLR3_UCHAR   c;

        c = (ANTLR3_UCHAR)(*((pANTLR3_UINT32)input->nextChar + la - 1));

        // Swap Endianess to Big Endian
        //
        return (c>>24) | ((c<<8) & 0x00FF0000) | ((c>>8) & 0x0000FF00) | (c<<24);
    }
}

/** \brief Return the input element assuming a UTF32 input in big endian byte order
 *
 * \param[in] input Input stream context pointer
 * \param[in] la 1 based offset of next input stream element
 *
 * \return Next input character in internal ANTLR3 encoding (UTF32)
 * \remark This is the same code as LE version but seprated in case there are better optimisations fo rendinan swap
 */
static ANTLR3_UCHAR 
antlr3UTF32LABE(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
    pANTLR3_INPUT_STREAM input;
	
    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    if	(( ((pANTLR3_UINT8)input->nextChar) + la - 1) >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {
		return	ANTLR3_CHARSTREAM_EOF;
    }
    else
    {
        ANTLR3_UCHAR   c;

        c = (ANTLR3_UCHAR)(*((pANTLR3_UINT32)input->nextChar + la - 1));

        // Swap Endianess to Little Endian
        //
        return (c>>24) | ((c<<8) & 0x00FF0000) | ((c>>8) & 0x0000FF00) | (c<<24);
    }
}


/// \brief Common function to setup function interface for a UTF8 input stream.
///
/// \param input Input stream context pointer
///
void 
antlr3UTF8SetupStream	(pANTLR3_INPUT_STREAM input)
{
    // Build a string factory for this stream. This is a UTF16 string factory which is a standard
    // part of the ANTLR3 string. The string factory is then passed through the whole chain of lexer->parser->tree->treeparser
    // and so on.
    //
    input->strFactory	= antlr3StringFactoryNew(input->encoding);

    // Generic API that does not care about endianess.
    //
    input->istream->consume	= antlr3UTF8Consume;	// Consume the next UTF32 character in the buffer
    input->istream->_LA         = antlr3UTF8LA;         // Return the UTF32 character at offset n (1 based)    
    input->charByteSize		= 0;	                // Size in bytes of characters in this stream.
}

// ------------------------------------------------------
// Following is from Unicode.org (see antlr3convertutf.c)
//

/// Index into the table below with the first byte of a UTF-8 sequence to
/// get the number of trailing bytes that are supposed to follow it.
/// Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
/// left as-is for anyone who may want to do such conversion, which was
/// allowed in earlier algorithms.
///
static const ANTLR3_UINT32 trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/// Magic values subtracted from a buffer value during UTF8 conversion.
/// This table contains as many values as there might be trailing bytes
/// in a UTF-8 sequence.
///
static const UTF32 offsetsFromUTF8[6] = 
    {   0x00000000UL, 0x00003080UL, 0x000E2080UL, 
	0x03C82080UL, 0xFA082080UL, 0x82082080UL 
    };

// End of Unicode.org tables
// -------------------------


/** \brief Consume the next character in a UTF8 input stream
 *
 * \param input Input stream context pointer
 */
static void
antlr3UTF8Consume(pANTLR3_INT_STREAM is)
{
    pANTLR3_INPUT_STREAM    input;
    ANTLR3_UINT32           extraBytesToRead;
    ANTLR3_UCHAR            ch;
    pANTLR3_UINT8           nextChar;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    nextChar = (pANTLR3_UINT8)input->nextChar;

    if	(nextChar < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {	
	// Indicate one more character in this line
	//
	input->charPositionInLine++;
	
        // Are there more bytes needed to make up the whole thing?
        //
        extraBytesToRead = trailingBytesForUTF8[*nextChar];

        if	(nextChar + extraBytesToRead >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
        {
            input->nextChar = (((pANTLR3_UINT8)input->data) + input->sizeBuf);
            return;
        }

        // Cases deliberately fall through (see note A in antlrconvertutf.c)
        // Legal UTF8 is only 4 bytes but 6 bytes could be used in old UTF8 so
        // we allow it.
        //
        ch  = 0;
       	switch (extraBytesToRead) {
	    case 5: ch += *nextChar++; ch <<= 6;
	    case 4: ch += *nextChar++; ch <<= 6;
	    case 3: ch += *nextChar++; ch <<= 6;
	    case 2: ch += *nextChar++; ch <<= 6;
	    case 1: ch += *nextChar++; ch <<= 6;
	    case 0: ch += *nextChar++;
	}

        // Magically correct the input value
        //
	ch -= offsetsFromUTF8[extraBytesToRead];
	if  (ch == input->newlineChar)
	{
	    /* Reset for start of a new line of input
	     */
	    input->line++;
	    input->charPositionInLine	= 0;
	    input->currentLine		= (void *)nextChar;
	}

        // Update input pointer
        //
        input->nextChar = nextChar;
    }
}
/** \brief Return the input element assuming a UTF8 input
 *
 * \param[in] input Input stream context pointer
 * \param[in] la 1 based offset of next input stream element
 *
 * \return Next input character in internal ANTLR3 encoding (UTF32)
 */
static ANTLR3_UCHAR 
antlr3UTF8LA(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
    pANTLR3_INPUT_STREAM    input;
    ANTLR3_UINT32           extraBytesToRead;
    ANTLR3_UCHAR            ch;
    pANTLR3_UINT8           nextChar;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    nextChar = (pANTLR3_UINT8)input->nextChar;

    // Do we need to traverse forwards or backwards?
    // - LA(0) is treated as LA(1) and we assume that the nextChar is
    //   already positioned.
    // - LA(n+) ; n>1 means we must traverse forward n-1 characters catering for UTF8 encoding
    // - LA(-n) means we must traverse backwards n chracters
    //
    if (la > 1) {

        // Make sure that we have at least one character left before trying to
        // loop through the buffer.
        //
        if	(nextChar < (((pANTLR3_UINT8)input->data) + input->sizeBuf))
        {	
            // Now traverse n-1 characters forward
            //
            while (--la > 0)
            {
                // Does the next character require trailing bytes?
                // If so advance the pointer by that many bytes as well as advancing
                // one position for what will be at least a single byte character.
                //
                nextChar += trailingBytesForUTF8[*nextChar] + 1;

                // Does that calculation take us past the byte length of the buffer?
                //
                if	(nextChar >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
                {
                    return ANTLR3_CHARSTREAM_EOF;
                }
            }
        }
        else
        {
            return ANTLR3_CHARSTREAM_EOF;
        }
    }
    else
    {
        // LA is negative so we decrease the pointer by n character positions
        //
        while   (nextChar > (pANTLR3_UINT8)input->data && la++ < 0)
        {
            // Traversing backwards in UTF8 means decermenting by one
            // then continuing to decrement while ever a character pattern
            // is flagged as being a trailing byte of an encoded code point.
            // Trailing UTF8 bytes always start with 10 in binary. We assumne that
            // the UTF8 is well formed and do not check boundary conditions
            //
            nextChar--;
            while ((*nextChar & 0xC0) == 0x80)
            {
                nextChar--;
            }
        }
    }

    // nextChar is now pointing at the UTF8 encoded character that we need to
    // decode and return.
    //
    // Are there more bytes needed to make up the whole thing?
    //
    extraBytesToRead = trailingBytesForUTF8[*nextChar];
    if	(nextChar + extraBytesToRead >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {
        return ANTLR3_CHARSTREAM_EOF;
    }

    // Cases deliberately fall through (see note A in antlrconvertutf.c)
    // 
    ch  = 0;
    switch (extraBytesToRead) {
            case 5: ch += *nextChar++; ch <<= 6;
            case 4: ch += *nextChar++; ch <<= 6;
            case 3: ch += *nextChar++; ch <<= 6;
            case 2: ch += *nextChar++; ch <<= 6;
            case 1: ch += *nextChar++; ch <<= 6;
            case 0: ch += *nextChar++;
    }

    // Magically correct the input value
    //
    ch -= offsetsFromUTF8[extraBytesToRead];

    return ch;
}

// EBCDIC to ASCII conversion table
//
// This for EBCDIC EDF04 translated to ISO-8859.1 which is the usually accepted POSIX
// translation and the character tables are published all over the interweb.
// 
const ANTLR3_UCHAR e2a[256] =
{
    0x00, 0x01, 0x02, 0x03, 0x85, 0x09, 0x86, 0x7f,
    0x87, 0x8d, 0x8e, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x8f, 0x0a, 0x08, 0x97,
    0x18, 0x19, 0x9c, 0x9d, 0x1c, 0x1d, 0x1e, 0x1f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x92, 0x17, 0x1b,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07, 
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04,
    0x98, 0x99, 0x9a, 0x9b, 0x14, 0x15, 0x9e, 0x1a,
    0x20, 0xa0, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5,
    0xe7, 0xf1, 0x60, 0x2e, 0x3c, 0x28, 0x2b, 0x7c,
    0x26, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef,
    0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x9f,
    0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5,
    0xc7, 0xd1, 0x5e, 0x2c, 0x25, 0x5f, 0x3e, 0x3f,
    0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf,
    0xcc, 0xa8, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22,
    0xd8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1,
    0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
    0x71, 0x72, 0xaa, 0xba, 0xe6, 0xb8, 0xc6, 0xa4,
    0xb5, 0xaf, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0xdd, 0xde, 0xae,
    0xa2, 0xa3, 0xa5, 0xb7, 0xa9, 0xa7, 0xb6, 0xbc,
    0xbd, 0xbe, 0xac, 0x5b, 0x5c, 0x5d, 0xb4, 0xd7,
    0xf9, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0xad, 0xf4, 0xf6, 0xf2, 0xf3, 0xf5,
    0xa6, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
    0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xdb, 0xfa, 0xff,
    0xd9, 0xf7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0xb3, 0x7b, 0xdc, 0x7d, 0xda, 0x7e
};

/// \brief Common function to setup function interface for a EBCDIC input stream.
///
/// \param input Input stream context pointer
///
void 
antlr3EBCDICSetupStream	(pANTLR3_INPUT_STREAM input)
{
    // EBCDIC streams can use the standard 8 bit string factory
    //
    input->strFactory	= antlr3StringFactoryNew(input->encoding);

    // Generic API that does not care about endianess.
    //
    input->istream->_LA         = antlr3EBCDICLA;       // Return the UTF32 character at offset n (1 based)    
    input->charByteSize		= 1;	                // Size in bytes of characters in this stream.
}

/// \brief Return the input element assuming an 8 bit EBCDIC input
///
/// \param[in] input Input stream context pointer
/// \param[in] la 1 based offset of next input stream element
///
/// \return Next input character in internal ANTLR3 encoding (UTF32) after translation
///         from EBCDIC to ASCII
///
static ANTLR3_UCHAR 
antlr3EBCDICLA(pANTLR3_INT_STREAM is, ANTLR3_INT32 la)
{
    pANTLR3_INPUT_STREAM input;

    input   = ((pANTLR3_INPUT_STREAM) (is->super));

    if	(( ((pANTLR3_UINT8)input->nextChar) + la - 1) >= (((pANTLR3_UINT8)input->data) + input->sizeBuf))
    {
        return	ANTLR3_CHARSTREAM_EOF;
    }
    else
    {
        // Translate the required character via the constant conversion table
        //
        return	e2a[(*((pANTLR3_UINT8)input->nextChar + la - 1))];
    }
}