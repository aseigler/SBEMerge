// SBEMerge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <atlbase.h>
#include <sbe.h>
#include <initguid.h>
#include <dshow.h>
#include <wmsdk.h>
#include <intsafe.h>

STDMETHODIMP CopyMetadata(LPCWSTR wszSource, IStreamBufferRecComp* pRecComp)
{
	HRESULT hr, hr2 = S_OK;

	CComPtr<IFileSourceFilter> pSourceFile;

	hr = pSourceFile.CoCreateInstance(CLSID_StreamBufferRecordingAttributes);
	if (FAILED(hr))
		return hr;

	hr = pSourceFile->Load(wszSource, NULL);
	if (FAILED(hr))
		return hr;

	CComQIPtr<IStreamBufferRecordingAttribute> pSourceRecordingAttributes(pSourceFile);
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

		try
		{
			wszAttributeName = new WCHAR[cchNameLength];
			abAttribute = new BYTE[cbAttribute];
			if ((NULL == wszAttributeName) || (NULL == abAttribute))
			{
				throw std::bad_alloc();
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
					break;

				cchMIME++;
				cchDesc++;
/*
				size_t cbMIME = 0;
				if (FAILED(SizeTMult(cchMIME, sizeof(WCHAR), &cbMIME)))
					break;

				size_t cbDesc = 0;
				if (FAILED(SizeTMult(cchDesc, sizeof(WCHAR), &cbDesc)))
					break;

				size_t cb = 0;
				if (FAILED(SizeTAdd(cb, cbMIME, &cb)))
					break;

				if (FAILED(SizeTAdd(cb, cbDesc, &cb)))
					break;

				if (FAILED(SizeTAdd(cb, RTL_FIELD_SIZE(WM_PICTURE,bPictureType), &cb)))
					break;

				if (FAILED(SizeTAdd(cb, RTL_FIELD_SIZE(WM_PICTURE,dwDataLen), &cb)))
					break;

				DWORD dwCB = 0;
				if (FAILED(SizeTToDWord(cb, &dwCB)))
					break;

				if (FAILED(DWordAdd(dwCB, WMPicture->dwDataLen, &dwCB)))
					break;

				if (FAILED(DWordToWord(dwCB, &cbAttribute)))
					break;*/
				//INTEGER OVERFLOW POSSIBLE HERE!
				cbAttribute =	(cchMIME + cchDesc) * sizeof(WCHAR)			// pwszMIMEType, pwszDescription
								+ RTL_FIELD_SIZE(WM_PICTURE,bPictureType)	// bPictureType
								+ RTL_FIELD_SIZE(WM_PICTURE,dwDataLen)		// dwDataLen
								+ WMPicture->dwDataLen;						// pbData
	
				BYTE* pbAttribute = new BYTE[cbAttribute];
				if (NULL == pbAttribute)
				{
					hr = E_OUTOFMEMORY;
					break;
				}

				ZeroMemory(pbAttribute, cbAttribute);

				hr = StringCchCopy((LPWSTR)(pbAttribute), cchMIME, WMPicture->pwszMIMEType);
				if FAILED(hr)
					break;

				pbAttribute += (cchMIME) * sizeof(WCHAR);
				
				*pbAttribute = WMPicture->bPictureType;

				pbAttribute += RTL_FIELD_SIZE(WM_PICTURE,bPictureType);

				hr = StringCchCopy((LPWSTR)(pbAttribute), cchDesc, WMPicture->pwszDescription);
				if FAILED(hr)
					break;
				
				pbAttribute += (cchDesc) * sizeof(WCHAR);

				*(DWORD*)(pbAttribute) = WMPicture->dwDataLen;
				
				pbAttribute += RTL_FIELD_SIZE(WM_PICTURE,dwDataLen);

				CopyMemory(pbAttribute, WMPicture->pbData, WMPicture->dwDataLen);

				pbAttribute = pbAttribute - cbAttribute + WMPicture->dwDataLen;
				
				delete[] abAttribute;
				abAttribute = pbAttribute;
			}
		}

		catch (std::bad_alloc &ba)
		{
			if (NULL != wszAttributeName)
				delete[] wszAttributeName;

			if (NULL != abAttribute)
				delete[] abAttribute;
		}

		if (SUCCEEDED(hr)) 
		{
			hr2 = pDestRecordingAttributes->SetAttribute(0, wszAttributeName, datatype, abAttribute, cbAttribute);
		}

		if (NULL != wszAttributeName)
			delete[] wszAttributeName;

		if (NULL != abAttribute)
			delete[] abAttribute;
	}

	pSourceRecordingAttributes.Release();
	pDestRecordingAttributes.Release();
	pSourceFile.Release();

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

