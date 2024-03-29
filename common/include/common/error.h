#ifndef OTRIX_ERROR_H
#define OTRIX_ERROR_H

typedef enum
{
    E_OK,
    E_NODEV,
    E_NOMEM,
    E_INVAL,
    E_TOUT,
    E_UNREACHABLE,
    E_NOIMPL,
    E_ADDRINUSE,
    E_PIPE,
} kerror_t;

#endif // OTRIX_ERROR_H
