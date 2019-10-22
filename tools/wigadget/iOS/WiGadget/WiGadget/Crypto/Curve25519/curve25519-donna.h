#ifndef	__curve25519_donnaDotH__
#define	__curve25519_donnaDotH__

#ifdef	__cplusplus
	extern "C" {
#endif

void curve25519_donna( unsigned char *outKey, const unsigned char *inSecret, const unsigned char *inBasePoint );

#ifdef	__cplusplus
	}
#endif

#endif	// __curve25519_donnaDotH__
