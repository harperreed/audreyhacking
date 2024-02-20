/*
 *      WinSOS NT utility for writing image files to floppy.
 *
 *      Toomas Kiisk <vix@cyber.ee>, Mar 27 1999
 *
 *	This program is provided AS IS, with NO WARRANTY, either
 *	expressed or implied.
 *
 *	This program or parts of it may be used, reused, abused,
 *	misused and disused for any purpose that is legal in
 *	your country. Names of the author and contributors may not
 *	be used to promote products derived from this software.
 *	Redistribution is not limited.
 *
 *
 *	Last modified Apr 1 1999
 *		some cleanups, 2 (harmless) bugs fixed and new ones
 *		introduced, of course :)
 *
 *      Modified by scythic@yahoo.com 
 *
 */

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


unsigned long int    dummy_variable_for_that_stupid_ioctl_function;
#define dummy dummy_variable_for_that_stupid_ioctl_function

#define LOCK_DRIVE( x ) ( DeviceIoControl( x, FSCTL_LOCK_VOLUME, \
			NULL, 0, NULL, 0, &dummy, NULL ) )

#define UNLOCK_DRIVE( x ) ( DeviceIoControl( x, FSCTL_UNLOCK_VOLUME, \
			NULL, 0, NULL, 0, &dummy, NULL ) )

#define GET_GEOMETRY( x, y ) ( DeviceIoControl( x, IOCTL_DISK_GET_DRIVE_GEOMETRY, \
			NULL, 0, y, sizeof( *y ), &dummy, NULL ) )

HANDLE  src = INVALID_HANDLE_VALUE, dst = INVALID_HANDLE_VALUE;
unsigned char    *buf = NULL;
unsigned int    bufsize;

void die( int r, char *fmt, ... );
void usage_exit( char *arg0 );


int main( int argc, char *argv[] )
{
	char dev[7], *fn;
	DISK_GEOMETRY   g;
	OSVERSIONINFO   v;
	unsigned long br, bw, bwt;

	printf("NTRW v2.00 \n");
	v.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	if ( ! GetVersionEx( &v ) )
		die( 2, "GetVersionEx()" );

	if ( v.dwPlatformId != VER_PLATFORM_WIN32_NT )
		die( 1, "This program won't work on Win95/98/ME.  Run on NT, 2K or later.\n" );

	if ( argc < 4 )
		usage_exit( argv[0] );

	fn = argv[2];
	sprintf( dev, "\\\\.\\%s", argv[3] );

	if (argv[1][0] == 'w')
	{
		src = CreateFile( fn, GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, 0, NULL );
		if ( src == INVALID_HANDLE_VALUE )
			die( 2, "Could not open %s", fn );

	}
	if (argv[1][0] == 'r')
	{
		src = CreateFile( fn, GENERIC_WRITE, FILE_SHARE_WRITE,
			NULL, CREATE_NEW, 0, NULL );
		if ( src == INVALID_HANDLE_VALUE )
			die( 2, "Could not open %s", fn );

	}

	dst = CreateFile( dev, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

	if ( dst == INVALID_HANDLE_VALUE )
		die( 2, "Could not open %s", argv[2] );

	if ( ! LOCK_DRIVE( dst ) )
		die( 2, "Could not lock %s", argv[2] );

	if ( ! GET_GEOMETRY( dst, &g ) )
		die( 2, "Could not get disk information for %s", argv[2] );

	bufsize = g.BytesPerSector * g.SectorsPerTrack;
	switch ( g.MediaType ) {
	case F5_1Pt2_512:
		printf( "5.25\", 1.2MB, 512 bytes/sector\n" );
		break;
	case F3_1Pt44_512:
		printf( "3.5\", 1.44MB, 512 bytes/sector\n" );
		break;
	case F3_2Pt88_512:
		printf( "3.5\", 2.88MB, 512 bytes/sector\n" );
		break;
	case F3_20Pt8_512:
		printf( "3.5\", 20.8MB, 512 bytes/sector\n" );
		break;
	case F3_720_512:
		printf( "3.5\", 720KB, 512 bytes/sector\n" );
		break;
	case F5_360_512:
		printf( "5.25\", 360KB, 512 bytes/sector\n" );
		break;
	case F5_320_512:
		printf( "5.25\", 320KB, 512 bytes/sector\n" );
		break;
	case F5_320_1024:
		printf( "5.25\", 320KB, 1024 bytes/sector\n" );
		break;
	case F5_180_512:
		printf( "5.25\", 180KB, 512 bytes/sector\n" );
		break;
	case F5_160_512:
		printf( "5.25\", 160KB, 512 bytes/sector\n" );
		break;
	case RemovableMedia:
                printf("Removeable media\n");
		printf("  Cylinders: %ld:%ld \n", g.Cylinders.HighPart,g.Cylinders.LowPart);
		printf("  TracksPerCylinder: %d \n", g.TracksPerCylinder);
		printf("  SectorsPerTrack: %d \n", g.SectorsPerTrack );
		printf("  BytesPerSector: %d \n", g.BytesPerSector );
                bufsize = 0;
 	 	if (argc > 4) bufsize = atoi(argv[4]);
		if (!bufsize) bufsize = 128*g.BytesPerSector;
		if ((bufsize%g.BytesPerSector) != 0)
		{
			die(1,"Bufsize is NOT a multiple of BytesPerSector! \n");		  
		}
                break;
	case FixedMedia:
		die( 1, "This program will not touch hard drives.\n" );
	default:
		die( 1, "Unknown media type\n" );
	}
	buf = VirtualAlloc( NULL, bufsize, MEM_COMMIT, PAGE_READWRITE );
	if ( buf == NULL )
		die( 2, "VirtualAlloc(): %d bytes", bufsize );

	printf( "bufsize is %d\n", bufsize );

        if (argv[1][0] == 'r')
	{
		int smaller_size;
		int result;

		for ( bwt=0 ;ReadFile( dst, buf, bufsize, &br, NULL ) && br; bwt += br ) {
			if ( ! WriteFile( src, buf, br, &bw, NULL ) )
				die( 2, "WriteFile(): %s", argv[ 2 ] );
		}	
		smaller_size = bufsize/2;
		while (smaller_size >= g.BytesPerSector)
		{
			printf("Trying %d        \r", smaller_size);
			result = ReadFile(dst, buf, smaller_size, &br, NULL);	
			printf("                 Res: %d bytes: %d\r", result, br);
			if (result && br)
			{
				bwt += br;
				if ( ! WriteFile( src, buf, br, &bw, NULL ) )
					die( 2, "WriteFile(): %s", argv[ 2 ] );
			}
			else
			{
				smaller_size >>= 1;
			}
		}		
	}
	if (argv[1][0] == 'w' )
	{
		for ( bwt=0 ;ReadFile( src, buf, bufsize, &br, NULL ) && br; bwt += br ) {
			if ( ! WriteFile( dst, buf, br, &bw, NULL ) )
				die( 2, "WriteFile(): %s", argv[ 2 ] );
		}	
	}

	printf( "%d bytes written\n", bwt );
	if ( GetLastError() != ERROR_SUCCESS )
		die( 2, "ReadFile(): %s", fn );

	die( 0, NULL );

	/* NOTREACHED	*/
	return 0;
}

void usage_exit( char *arg0 )
{
	char    *prog;

	prog = (prog = strrchr( arg0, '\\' ))?++prog:arg0;
	die( 1, "Usage: %s read/write file drive:\n", prog );
}

void die( int r, char *fmt, ... )
{
	char *msg, defmsg[] = "[FormatMessage failed]";
	va_list a;
	unsigned int err;

	err = GetLastError();
	if ( r > 1 ) { /* OS error	*/
		if ( ! FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,    err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&msg, 0, NULL ) )
			msg = defmsg;

	} else
		msg = NULL;


	if ( fmt != NULL ) {
		va_start( a, fmt );
		vfprintf( stderr, fmt, a );
		va_end( a );
	}

	if ( msg ) {
		fprintf( stderr, " -- %s\n", msg );
		if ( msg != defmsg )
			LocalFree( msg );
	}


	if ( src != INVALID_HANDLE_VALUE )
		CloseHandle( src );

	if ( dst != INVALID_HANDLE_VALUE ) {
		UNLOCK_DRIVE( dst );
		CloseHandle( dst );
	}

	if ( buf != NULL )
		VirtualFree( buf, bufsize, MEM_DECOMMIT );

	exit( r );
}
