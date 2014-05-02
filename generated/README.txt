* How to generated Interfaces

1) In ./modules/interfaces add a new header. It has to include a class definition like this. 
   Note that the DECLARE_REGISTER_INTERFACE(YourClassInterfaceImpl, is processed via regex

class YourClassInterfaceImpl : public grt::InterfaceImplBase
{
public:
  DECLARE_REGISTER_INTERFACE(YourClassInterfaceImpl,
                             DECLARE_INTERFACE_FUNCTION(YourClassInterfaceImpl::YourFunction));

  virtual int YourFunction()= 0;
};

2) Add your interface header from above to ./modules/interfaces/interfaces.h and make sure 
   you register your interface.

3) Make sure your module class implements that interface

4) Call generate.cmd in ./generated to generate the InterfaceModule in ./generated/grti

5) Only use this class to call your module

