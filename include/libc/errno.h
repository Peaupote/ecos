#ifndef _H_LIBC_ERRNO
#define _H_LIBC_ERRNO

#define NERRNO 135
const char * const err_list[NERRNO];
int errno;

enum error_kind {
    SUCC,               // Success
    EPERM,              // Operation not permitted
    ENOENT,             // No such file or directory
    ESRCH,              // No such process
    EINTR,              // Interrupted system call
    EIO,                // Input/output error
    ENXIO,              // No such device or address
    E2BIG,              // Argument list too long
    ENOEXEC,            // Exec format error
    EBADF,              // Bad file descriptor
    ECHILD,             //0 No child processes
    EAGAIN,             //1 Resource temporarily unavailable
    ENOMEM,             //2 Cannot allocate memory
    EACCES,             //3 Permission denied
    EFAULT,             //4 Bad address
    ENOTBLK,            //5 Block device required
    EBUSY,              //6 Device or resource busy
    EEXIST,             //7 File exists
    EXDEV,              //8 Invalid cross-device link
    ENODEV,             //9 No such device
    ENOTDIR,            //0 Not a directory
    EISDIR,             //1 Is a directory
    EINVAL,             //2 Invalid argument
    ENFILE,             //3 Too many open files in system
    EMFILE,             //4 Too many open files
    ENOTTY,             //5 Inappropriate ioctl for device
    ETXTBSY,            //6 Text file busy
    EFBIG,              //7 File too large
    ENOSPC,             //8 No space left on device
    ESPIPE,             //9 Illegal seek
    EROFS,              //0 Read-only file system
    EMLINK,             //1 Too many links
    EPIPE,              //2 Broken pipe
    EDOM,               //3 Numerical argument out of domain
    ERANGE,             //4 Numerical result out of range
    EDEADLK,            //5 Resource deadlock avoided
    ENAMETOOLONG,       //6 File name too long
    ENOLCK,             //7 No locks available
    ENOSYS,             //8 Function not implemented
    ENOTEMPTY,          //9 Directory not empty
    ELOOP,              //0 Too many levels of symbolic links
    EWOULDBLOCK,        //1 Resource temporarily unavailable
    ENOMSG,             //2 No message of desired type
    EIDRM,              //3 Identifier removed
    ECHRNG,             //4 Channel number out of range
    EL2NSYNC,           //5 Level 2 not synchronized
    EL3HLT,             //6 Level 3 halted
    EL3RST,             //7 Level 3 reset
    ELNRNG,             //8 Link number out of range
    EUNATCH,            //9 Protocol driver not attached
    ENOCSI,             //0 No CSI structure available
    EL2HLT,             //1 Level 2 halted
    EBADE,              //2 Invalid exchange
    EBADR,              //3 Invalid request descriptor
    EXFULL,             //4 Exchange full
    ENOANO,             //5 No anode
    EBADRQC,            //6 Invalid request code
    EBADSLT,            //7 Invalid slot
    EDEADLOCK,          //5 Resource deadlock avoided
    EBFONT,             //9 Bad font file format
    ENOSTR,             //0 Device not a stream
    ENODATA,            //1 No data available
    ETIME,              //2 Timer expired
    ENOSR,              //3 Out of streams resources
    ENONET,             //4 Machine is not on the network
    ENOPKG,             //5 Package not installed
    EREMOTE,            //6 Object is remote
    ENOLINK,            //7 Link has been severed
    EADV,               //8 Advertise error
    ESRMNT,             //9 Srmount error
    ECOMM,              //0 Communication error on send
    EPROTO,             //1 Protocol error
    EMULTIHOP,          //2 Multihop attempted
    EDOTDOT,            //3 RFS specific error
    EBADMSG,            //4 Bad message
    EOVERFLOW,          //5 Value too large for defined data type
    ENOTUNIQ,           //6 Name not unique on network
    EBADFD,             //7 File descriptor in bad state
    EREMCHG,            //8 Remote address changed
    ELIBACC,            //9 Can not access a needed shared library
    ELIBBAD,            //0 Accessing a corrupted shared library
    ELIBSCN,            //1 .lib section in a.out corrupted
    ELIBMAX,            //2 Attempting to link in too many shared libraries
    ELIBEXEC,           //3 Cannot exec a shared library directly
    EILSEQ,             //4 Invalid or incomplete multibyte or wide character
    ERESTART,           //5 Interrupted system call should be restarted
    ESTRPIPE,           //6 Streams pipe error
    EUSERS,             //7 Too many users
    ENOTSOCK,           //8 Socket operation on non-socket
    EDESTADDRREQ,       //9 Destination address required
    EMSGSIZE,           //0 Message too long
    EPROTOTYPE,         //1 Protocol wrong type for socket
    ENOPROTOOPT,        //2 Protocol not available
    EPROTONOSUPPORT,    //3 Protocol not supported
    ESOCKTNOSUPPORT,    //4 Socket type not supported
    EOPNOTSUPP,         //5 Operation not supported
    EPFNOSUPPORT,       //6 Protocol family not supported
    EAFNOSUPPORT,       //7 Address family not supported by protocol
    EADDRINUSE,         //8 Address already in use
    EADDRNOTAVAIL,      //9 Cannot assign requested address
    ENETDOWN,           //00 Network is down
    ENETUNREACH,        //01 Network is unreachable
    ENETRESET,          //02 Network dropped connection on reset
    ECONNABORTED,       //03 Software caused connection abort
    ECONNRESET,         //04 Connection reset by peer
    ENOBUFS,            //05 No buffer space available
    EISCONN,            //06 Transport endpoint is already connected
    ENOTCONN,           //07 Transport endpoint is not connected
    ESHUTDOWN,          //08 Cannot send after transport endpoint shutdown
    ETOOMANYREFS,       //09 Too many references: cannot splice
    ETIMEDOUT,          //10 Connection timed out
    ECONNREFUSED,       //11 Connection refused
    EHOSTDOWN,          //12 Host is down
    EHOSTUNREACH,       //13 No route to host
    EALREADY,           //14 Operation already in progress
    EINPROGRESS,        //15 Operation now in progress
    ESTALE,             //16 Stale file handle
    EUCLEAN,            //17 Structure needs cleaning
    ENOTNAM,            //18 Not a XENIX named type file
    ENAVAIL,            //19 No XENIX semaphores available
    EISNAM,             //20 Is a named type file
    EREMOTEIO,          //21 Remote I/O error
    EDQUOT,             //22 Disk quota exceeded
    ENOMEDIUM,          //23 No medium found
    EMEDIUMTYPE,        //24 Wrong medium type
    ECANCELED,          //25 Operation canceled
    ENOKEY,             //26 Required key not available
    EKEYEXPIRED,        //27 Key has expired
    EKEYREVOKED,        //28 Key has been revoked
    EKEYREJECTED,       //29 Key was rejected by service
    EOWNERDEAD,         //30 Owner died
    ENOTRECOVERABLE,    //31 State not recoverable
    ERFKILL,            //32 Operation not possible due to RF-kill
    EHWPOISON,          //33 Memory page has hardware error
    ENOTSUP,            //5 Operation not supported
};

#endif
