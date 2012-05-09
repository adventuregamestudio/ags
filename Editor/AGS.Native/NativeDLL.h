

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Sun Nov 19 18:19:54 2006
 */
/* Compiler settings for .\NativeDLL.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __NativeDLL_h__
#define __NativeDLL_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __INativeInterface_FWD_DEFINED__
#define __INativeInterface_FWD_DEFINED__
typedef interface INativeInterface INativeInterface;
#endif 	/* __INativeInterface_FWD_DEFINED__ */


#ifndef __NativeInterface_FWD_DEFINED__
#define __NativeInterface_FWD_DEFINED__

#ifdef __cplusplus
typedef class NativeInterface NativeInterface;
#else
typedef struct NativeInterface NativeInterface;
#endif /* __cplusplus */

#endif 	/* __NativeInterface_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __INativeInterface_INTERFACE_DEFINED__
#define __INativeInterface_INTERFACE_DEFINED__

/* interface INativeInterface */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INativeInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("29A92820-4465-40AB-A752-9D059DAEC470")
    INativeInterface : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DrawGUI( 
            /* [in] */ LONG hDC,
            /* [in] */ LONG x,
            /* [in] */ LONG y) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetGUIDefinition( 
            /* [in] */ BYTE *GuiData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INativeInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INativeInterface * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INativeInterface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INativeInterface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            INativeInterface * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            INativeInterface * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            INativeInterface * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            INativeInterface * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            INativeInterface * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DrawGUI )( 
            INativeInterface * This,
            /* [in] */ LONG hDC,
            /* [in] */ LONG x,
            /* [in] */ LONG y);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetGUIDefinition )( 
            INativeInterface * This,
            /* [in] */ BYTE *GuiData);
        
        END_INTERFACE
    } INativeInterfaceVtbl;

    interface INativeInterface
    {
        CONST_VTBL struct INativeInterfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INativeInterface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INativeInterface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INativeInterface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INativeInterface_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INativeInterface_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INativeInterface_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INativeInterface_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define INativeInterface_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define INativeInterface_DrawGUI(This,hDC,x,y)	\
    (This)->lpVtbl -> DrawGUI(This,hDC,x,y)

#define INativeInterface_SetGUIDefinition(This,GuiData)	\
    (This)->lpVtbl -> SetGUIDefinition(This,GuiData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE INativeInterface_Initialize_Proxy( 
    INativeInterface * This);


void __RPC_STUB INativeInterface_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE INativeInterface_DrawGUI_Proxy( 
    INativeInterface * This,
    /* [in] */ LONG hDC,
    /* [in] */ LONG x,
    /* [in] */ LONG y);


void __RPC_STUB INativeInterface_DrawGUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE INativeInterface_SetGUIDefinition_Proxy( 
    INativeInterface * This,
    /* [in] */ BYTE *GuiData);


void __RPC_STUB INativeInterface_SetGUIDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INativeInterface_INTERFACE_DEFINED__ */



#ifndef __NativeDLLLib_LIBRARY_DEFINED__
#define __NativeDLLLib_LIBRARY_DEFINED__

/* library NativeDLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_NativeDLLLib;

EXTERN_C const CLSID CLSID_NativeInterface;

#ifdef __cplusplus

class DECLSPEC_UUID("8FB6C6F0-7E2C-488A-8AA0-8527FD26D08E")
NativeInterface;
#endif
#endif /* __NativeDLLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


