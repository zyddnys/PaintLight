#pragma once

#include <string>
#include "DXUT.h"

class OpenFileDialog
{
public:
	std::wstring operator()()
	{
        std::wstring result;
        HRESULT hr = S_OK;// CoInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog *pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                pFileOpen->SetTitle(L"Open Image");
                //pFileOpen->SetOptions(L"Image files(*.jpg;*.png;*.bmp)|*.jpg;*.png;*.bmp");
                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem *pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                        // Display the file name to the user.
                        if (SUCCEEDED(hr))
                        {
                            //MessageBox(NULL, pszFilePath, L"File Path", MB_OK);
                            result = pszFilePath;
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
                pFileOpen->Release();
            }
            //CoUninitialize();
        }
        return result;
	}
};
