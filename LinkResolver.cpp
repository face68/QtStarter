#include "LinkResolver.h"

#include <QDebug>

#ifdef _WIN32
#  include <windows.h>
#  include <shobjidl.h>
#  include <objbase.h>
#  include <msi.h>
#  pragma comment(lib, "Ole32.lib")
#  pragma comment(lib, "Shell32.lib")
#  pragma comment(lib, "Msi.lib")
#endif
#include <QString>

ShortcutInfo resolveShortcut( const QString& lnkPath ) {
#ifndef _WIN32
	Q_UNUSED( lnkPath );
	return {};
#else
	ShortcutInfo info;

	HRESULT hr = CoInitialize( nullptr );
	if( FAILED( hr ) ) {
		// Fehler behandeln, z.B.:
		if( hr == S_FALSE ) {
			// COM wurde bereits initialisiert - das ist normalerweise OK
		}
		else {
			// Kritischer Fehler bei der COM-Initialisierung
			qWarning() << "Failed to initialize COM:" << hr;
			return {}; // Oder andere Fehlerbehandlung
		}
	}

	// 1) Erst MSI-Shortcut versuchen (liefert echten Zielpfad)
	WCHAR prod[ 39 ]{}, feat[ 128 ]{}, comp[ 39 ]{};
	if( MsiGetShortcutTargetW( reinterpret_cast< LPCWSTR >( lnkPath.utf16() ), prod, feat, comp ) == ERROR_SUCCESS ) {
		WCHAR buf[ MAX_PATH ];
		DWORD cch = MAX_PATH;
		INSTALLSTATE st = MsiGetComponentPathW( prod, comp, buf, &cch );
		if( st == INSTALLSTATE_LOCAL || st == INSTALLSTATE_SOURCE ) {
			info.path = QString::fromWCharArray( buf );
			info.isMsi = true;
		}
	}

	// 2) Zusaetzlich (oder Fallback) normale LNK-Felder lesen
	IShellLinkW* psl = nullptr;
	if( SUCCEEDED( CoCreateInstance( CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &psl ) ) ) && psl ) {
		IPersistFile* ppf = nullptr;
		if( SUCCEEDED( psl->QueryInterface( IID_PPV_ARGS( &ppf ) ) ) && ppf ) {
			if( SUCCEEDED( ppf->Load( reinterpret_cast< LPCWSTR >( lnkPath.utf16() ), STGM_READ ) ) ) {
				// Arguments + WorkingDir
				WCHAR w[ INFOTIPSIZE ]{};
				if( SUCCEEDED( psl->GetArguments( w, INFOTIPSIZE ) ) ) {
					info.arguments = QString::fromWCharArray( w );
				}
				if( SUCCEEDED( psl->GetWorkingDirectory( w, INFOTIPSIZE ) ) ) {
					info.workingDir = QString::fromWCharArray( w );
				}

				// Falls kein MSI-Ziel ermittelt wurde: normalen Pfad nehmen
				if( info.path.isEmpty() ) {
					WCHAR path[ MAX_PATH ]{};
					if( SUCCEEDED( psl->GetPath( path, MAX_PATH, nullptr, SLGP_UNCPRIORITY ) ) ) {
						info.path = QString::fromWCharArray( path );
					}
				}
			}
			ppf->Release();
		}
		psl->Release();
	}

	CoUninitialize();
	return info;
#endif
}
