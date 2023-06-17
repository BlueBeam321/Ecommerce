#ifndef _MxErrCodes_h_
#define _MxErrCodes_h_

#pragma once

/* error code */
#define	mxErr_PmtChanged				7		// pmt was changed, send the new stream information
#define	mxErr_ResetByPmtChange			6		// pmt was changed, reset all the links
#define	mxErr_InfoCollected				5		// information was collected
#define	mxErr_DataCollected				4		// data was collected
#define	mxErr_Stop						3		// stop operation
#define	mxErr_ReSync					2		// re-synchronize
#define	mxErr_Buffering					1		// buffering
#define	mxErr_None						0

#define	mxErr_Generic					-1
#define	mxErr_NotEnough					-2
#define	mxErr_Cancelled					-3
#define	mxErr_InvalidParam				-4
#define	mxErr_InvalidFunction			-5
#define	mxErr_InvalidFormat				-6
#define	mxErr_InvalidCall				-7
#define	mxErr_NoMemory					-8
#define	mxErr_TimeOut					-9
#define	mxErr_BadValue					-10
#define	mxErr_NotFound					-11
#define	mxErr_Duplicate					-12

/* file operation related error code */
#define	mxErr_FileBadName				-100	// Bad filename or volume name
#define	mxErr_FileMemeoryFull			-101	// Memory full (open) or file won't fit(load)
#define	mxErr_FileNotFound				-102	// File or directory not found; incomplete pathname
#define	mxErr_FileInvalid				-103	// File object is invalid
#define	mxErr_FileLocked				-104	// File is locked
#define	mxErr_FileBusy					-105	// One or more files are open, 
#define	mxErr_FileSizeSmall				-107	// File size is small to process

#define	mxErr_FileDuplicateName			-106	// Duplicate filename and version

#define	mxErr_FileInvalidForkRef		-110	// Invalid reference number
#define	mxErr_FileAlreadyOpenWrite		-111	// File already open for writing
#define	mxErr_FileNotOpen				-112	// File not open
#define	mxErr_FileEOF					-113	// Logical end-of-file reached
#define	mxErr_FileInvalidPosition		-114	// Attempt to position mark before the start of the file
#define	mxErr_FileTooManyOpen			-115	// Too many files open
#define	mxErr_FileIO					-116	// I/O error

#define	mxErr_FileVolumeFull			-120	// Disk or volume full
#define	mxErr_FileVolumeNotFound		-121	// Volume not found
#define	mxErr_FileVolumeHardwareLocked	-122	// Volume is locked through hardware
#define	mxErr_FileVolumeSoftwareLocked	-123	// Volume is locked through software

#define	mxErr_FileNotPermission			-130
#define	mxErr_FileNotDrive				-131
#define	mxErr_FileAccessDenied			-132

#define	mxErr_FileDirectoryFull			-144	// File directory full

#define	mxErr_FileUnknown				-150

/* */
#define	mxErr_OnlyLocalPlayable			-200	// File must be converted to be registered into a server
#define	mxErr_FileCanRestore			-201
#define	mxErr_SystemResourceError		-202
#define mxErr_InvalidAudioDelFile		-203
#define	mxErr_SegCanRestore				-204
#define	mxErr_OnlyRemuxable				-205	// File must be remuxed into other format available in server

#define	mxErr_SockGeneric				-206
#define	mxErr_SockTimeout				-207
#define	mxErr_SockStopByUser			-208
#define	mxErr_SockIdle					-209

#define	mxErr_SockInit					-210
#define	mxErr_SockCreate				-211
#define	mxErr_SockRecvBuf				-212
#define	mxErr_SockSendBuf				-213
#define	mxErr_SockReuseAddr				-214
#define	mxErr_SockNonBlock				-215
#define	mxErr_SockKeepAlive				-216
#define	mxErr_SockNoDelay				-217
#define	mxErr_SockInvalidAddr			-218
#define	mxErr_SockKeepAliveTime			-219
#define	mxErr_SockMultiIF				-220
#define	mxErr_SockMultiTTL				-221
#define	mxErr_SockMultiMembership		-222
#define	mxErr_SockNoSigPipe				-223

#define	mxErr_SockConnect				-230
#define	mxErr_SockBind					-231
#define	mxErr_SockListen				-232
#define	mxErr_SockAccept				-233

#define	mxErr_SockRecvGracefulClose		-240
#define	mxErr_SockAuth					-250
#define	mxErr_CipherCreate				-251
#define	mxErr_CipherSetKey				-252
#define mxErr_CipherInvalid				-253
#define mxErr_CipherEnc					-254
#define mxErr_CipherDec					-255
#define mxErr_CipherGenKey				-256
#define mxErr_CipherSetVersion			-257
#define mxErr_CipherGetVersion			-258

#endif

