#ifdef _WIN32
#  include <windows.h>
#  include <shobjidl.h>
#  include <shlguid.h>
#  include <objbase.h>
#  include <combaseapi.h>
#endif
#include <QString>

QString resolveShortcutTarget( const QString& lnkPath ) {
#ifndef _WIN32
	Q_UNUSED( lnkPath );
	return QString();
#else
	HRESULT hr;
	CoInitialize( nullptr );
	IShellLinkW* psl = nullptr;
	QString result;

	hr = CoCreateInstance( CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, ( void** )&psl );
	if( SUCCEEDED( hr ) && psl ) {
		IPersistFile* ppf = nullptr;
		hr = psl->QueryInterface( IID_IPersistFile, ( void** )&ppf );
		if( SUCCEEDED( hr ) && ppf ) {
			hr = ppf->Load( reinterpret_cast< LPCWSTR >( lnkPath.utf16() ), STGM_READ );
			if( SUCCEEDED( hr ) ) {
				wchar_t szPath[ MAX_PATH ] = { 0 };
				hr = psl->GetPath( szPath, MAX_PATH, nullptr, SLGP_RAWPATH );
				if( SUCCEEDED( hr ) ) result = QString::fromWCharArray( szPath );
			}
			ppf->Release();
		}
		psl->Release();
	}
	CoUninitialize();
	return result;
#endif
}