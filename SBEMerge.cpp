// SBEMerge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <atlbase.h>
#include <sbe.h>
#include <initguid.h>
#include <dshow.h>
#include <wmsdk.h>

STDMETHODIMP CopyMetadata(LPCWSTR wszSource, IStreamBufferRecComp* pRecComp)
{
	HRESULT hr = S_OK;

	CComPtr <IGraphBuilder> pGraph;
	hr = pGraph.CoCreateInstance(CLSID_FilterGraph);
	if (FAILED(hr))
		return hr;

	CComPtr<IBaseFilter> pSource;
	hr = pSource.CoCreateInstance(CLSID_StreamBufferSource);
	if (FAILED(hr))
		return hr;

	hr = pGraph->AddFilter(pSource, L"Source");
	if (FAILED(hr))
		return hr;

	CComQIPtr<IFileSourceFilter> pSourceFile(pSource);
	hr = pSourceFile->Load(wszSource, NULL);
	if (FAILED(hr))
		return hr;
	pSourceFile.Release();

	CComQIPtr<IStreamBufferRecordingAttribute> pSourceRecordingAttributes(pSource);
	CComQIPtr<IStreamBufferRecordingAttribute> pDestRecordingAttributes(pRecComp);

	WORD wAttributes;
	hr = pSourceRecordingAttributes->GetAttributeCount(0, &wAttributes);
	if (FAILED(hr))
		return hr;

	for (WORD i = 0; i < wAttributes; i++) 
	{
        WCHAR *wszAttributeName = NULL;
        WORD cchNameLength = 0;
        STREAMBUFFER_ATTR_DATATYPE datatype;

        BYTE *abAttribute = NULL;
        WORD cbAttribute = 0;

        hr = pSourceRecordingAttributes->GetAttributeByIndex(i, 0,
            wszAttributeName, &cchNameLength,
            &datatype,
            abAttribute, &cbAttribute);

		if (FAILED(hr))
			return hr;

		wszAttributeName = new WCHAR[cchNameLength];
		if (NULL == wszAttributeName)
		{
			return E_OUTOFMEMORY;
		}

		abAttribute = new BYTE[cbAttribute];
		if (NULL == abAttribute)
		{
			delete[] wszAttributeName; 
			return E_OUTOFMEMORY;
		}
		
		hr = pSourceRecordingAttributes->GetAttributeByIndex(i, 0,
			wszAttributeName, &cchNameLength,
			&datatype,
			abAttribute, &cbAttribute);

              if (CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, wszAttributeName, -1, g_wszWMPicture, -1) == CSTR_EQUAL)
              {                    
                     WM_PICTURE*  WMPicture;
                     WMPicture = reinterpret_cast <WM_PICTURE*> (abAttribute);
                     
                     size_t cchMIME, cchDesc;
                     if (FAILED(StringCchLength(WMPicture->pwszMIMEType, cbAttribute, &cchMIME))
                                  || FAILED(StringCchLength(WMPicture->pwszDescription, cbAttribute, &cchDesc)))
                            return hr;

                     cchMIME++;
                     cchDesc++;

                     cbAttribute = RTL_FIELD_SIZE(WM_PICTURE,pwszMIMEType)
												+ RTL_FIELD_SIZE(WM_PICTURE, pwszDescription)
												+ (cchMIME + cchDesc) * sizeof(WCHAR)                    // pwszMIMEType, pwszDescription
                                                + RTL_FIELD_SIZE(WM_PICTURE,bPictureType)       // bPictureType
                                                + RTL_FIELD_SIZE(WM_PICTURE,dwDataLen)          // dwDataLen
												+ RTL_FIELD_SIZE(WM_PICTURE,pbData)
                                                + WMPicture->dwDataLen;                                       // pbData

                     BYTE* pbAttribute = new BYTE[cbAttribute];
                     if (NULL == pbAttribute)
                     {
                           hr = E_OUTOFMEMORY;
                           break;
                     }

					 BYTE* pbT = pbAttribute;

                     ZeroMemory(pbAttribute, cbAttribute);
					
					 pbAttribute += RTL_FIELD_SIZE(WM_PICTURE,pwszMIMEType);

                     hr = StringCchCopy((LPWSTR)(pbAttribute), cchMIME, WMPicture->pwszMIMEType);
                     if FAILED(hr)
                           break;

					 pbAttribute += (cchMIME) * sizeof(WCHAR);

					 (BYTE*)(pbAttribute - RTL_FIELD_SIZE(WM_PICTURE,pwszMIMEType)) = pbAttribute;
                     
                     *pbAttribute = WMPicture->bPictureType;
                     pbAttribute += RTL_FIELD_SIZE(WM_PICTURE,bPictureType);

                     hr = StringCchCopy((LPWSTR)(pbAttribute), cchDesc, WMPicture->pwszDescription);
                     if FAILED(hr)
                           break;
                     
                     pbAttribute += (cchDesc) * sizeof(WCHAR);

					 *(BYTE*)(pbAttribute - RTL_FIELD_SIZE(WM_PICTURE,pwszDescription)) = pbAttribute;

                     *(DWORD*)(pbAttribute) = WMPicture->dwDataLen;
                     
                     pbAttribute += RTL_FIELD_SIZE(WM_PICTURE,dwDataLen);

					 pbAttribute += RTL_FIELD_SIZE(WM_PICTURE,pbData);

                     CopyMemory(pbAttribute, WMPicture->pbData, WMPicture->dwDataLen);

					 *(BYTE*)(pbAttribute - RTL_FIELD_SIZE(WM_PICTURE,pbData)) = pbAttribute;

                     delete[] abAttribute;
                     abAttribute = pbT;
              }
        
		if (SUCCEEDED(hr)) 
		{
			pDestRecordingAttributes->SetAttribute(0, wszAttributeName, datatype, abAttribute, cbAttribute);
		}
		delete[] wszAttributeName;
		delete[] abAttribute;
	}
	
	pSourceRecordingAttributes.Release();
	pDestRecordingAttributes.Release();
	pSource.Release();
	pGraph.Release();

return hr;
}

int _tmain(int argc, _TCHAR* argv[])
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
		return hr;

	CComPtr<IStreamBufferRecComp> pRecComp;
	hr = pRecComp.CoCreateInstance(CLSID_StreamBufferComposeRecording);
	if (FAILED(hr))
		return hr;

	int i = 1;
	hr = pRecComp->Initialize(argv[argc - 1], argv[i]);
	if (FAILED(hr))
		return hr;

	hr = CopyMetadata(argv[i], pRecComp);
	if (FAILED(hr))
		return hr;

	while (i <= (argc - 2))
	{
		hr = pRecComp->Append(argv[i]);
		if (FAILED(hr))
			return hr;
		i++;
	}

	pRecComp.Release();
	CoUninitialize();
	return hr;
}

