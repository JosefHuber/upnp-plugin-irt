/* 
 * File:   filehandle.h
 * Author: savop
 *
 * Created on 15. Oktober 2009, 10:49
 */

#ifndef _FILEHANDLE_H
#define	_FILEHANDLE_H

#include <upnp/upnp.h>
#include "../common.h"

/**
 * Interface for File Handles
 * 
 * This class is a pure virtual class to act as an interface for file handles
 * used by the webserver.
 */
class cFileHandle {
public:
    /**
     * Opens the file
     *
     * Opens the file at the given mode. These can be:
     * - \b UPNP_READ, to read from the file
     * - \b UPNP_WRITE, to write to the file
     *
     * @param mode The file mode, i.e. one of the following
     *              - \b UPNP_READ
     *              - \b UPNP_WRITE
     */
    virtual void open(
        UpnpOpenFileMode mode ///< The file mode, i.e. one of the following
                              ///< - \b UPNP_READ
                              ///< - \b UPNP_WRITE
    ) = 0;
    /**
     * Reads from the file
     *
     * Reads from the file a certain amount of bytes and stores them in a buffer
     *
     * @return returns
     * - \b <0, in case of an error
     * - \b 0, when reading was successful
     *
     * @param buf The char buffer
     * @param buflen The size of the buffer
     */
    virtual int read(
        char* buf,              ///< The char buffer
        size_t buflen           ///< The size of the buffer
    ) = 0;
    /**
     * Writes to the file
     *
     * Writes to the file a certain amount of bytes which are stored in a buffer
     *
     * @return returns
     * - \b <0, in case of an error
     * - \b 0, when reading was successful
     *
     * @param buf The char buffer
     * @param buflen The size of the buffer
     */
    virtual int write(
        char* buf,              ///< The char buffer
        size_t buflen           ///< The size of the buffer
    ) = 0;
    /**
     * Seeks in the file
     *
     * Seeks in the file where the offset is the relativ position depending on
     * the second parameter. This means, in case of
     *
     * - \b SEEK_SET, the offset is relative to the beginning of the file
     * - \b SEEK_CUR, it is relative to the current position or
     * - \b SEEK_END, relative to the end of the file.
     *
     * @return returns
     * - \b <0, in case of an error
     * - \b 0, when reading was successful
     *
     * @param offset The byte offset in the file
     * @param whence one of the following
     *               - \b SEEK_SET,
     *               - \b SEEK_CUR,
     *               - \b SEEK_END
     */
    virtual int seek(
        off_t offset,           ///< The byte offset in the file
        int whence              ///< one of the following
                                ///< - \b SEEK_SET,
                                ///< - \b SEEK_CUR,
                                ///< - \b SEEK_END
    ) = 0;
    /**
     * Closes the open file
     *
     * This will close open file handles and frees the memory obtained by it.
     */
    virtual void close() = 0;
    virtual ~cFileHandle(){};
private:
};

#endif	/* _FILEHANDLE_H */

