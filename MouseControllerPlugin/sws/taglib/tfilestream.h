/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#ifndef TAGLIB_FILESTREAM_H
#define TAGLIB_FILESTREAM_H

#include "taglib_export.h"
#include "taglib.h"
#include "tbytevector.h"
#include "tiostream.h"

namespace TagLib {

  class String;
  class Tag;
  class AudioProperties;

  //! A file class with some useful methods for tag manipulation

  /*!
   * This class is a basic file class with some methods that are particularly
   * useful for tag editors.  It has methods to take advantage of
   * ByteVector and a binary search method for finding patterns in a file.
   */

  class TAGLIB_EXPORT FileStream : public IOStream
  {
  public:
    /*!
     * Construct a File object and opens the \a file.  \a file should be a
     * be a C-string in the local file system encoding.
     */
    FileStream(FileName file, bool openReadOnly = false);

    /*!
     * Destroys this FileStream instance.
     */
    virtual ~FileStream();

    /*!
     * Returns the file name in the local file system encoding.
     */
    FileName name() const;

    /*!
     * Reads a block of size \a length at the current get pointer.
     */
    ByteVector readBlock(unsigned long length);

    /*!
     * Attempts to write the block \a data at the current get pointer.  If the
     * file is currently only opened read only -- i.e. readOnly() returns true --
     * this attempts to reopen the file in read/write mode.
     *
     * \note This should be used instead of using the streaming output operator
     * for a ByteVector.  And even this function is significantly slower than
     * doing output with a char[].
     */
    void writeBlock(const ByteVector &data);

    /*!
     * Insert \a data at position \a start in the file overwriting \a replace
     * bytes of the original content.
     *
     * \note This method is slow since it requires rewriting all of the file
     * after the insertion point.
     */
    void insert(const ByteVector &data, unsigned long start = 0, unsigned long replace = 0);

    /*!
     * Removes a block of the file starting a \a start and continuing for
     * \a length bytes.
     *
     * \note This method is slow since it involves rewriting all of the file
     * after the removed portion.
     */
    void removeBlock(unsigned long start = 0, unsigned long length = 0);

    /*!
     * Returns true if the file is read only (or if the file can not be opened).
     */
    bool readOnly() const;

    /*!
     * Since the file can currently only be opened as an argument to the
     * constructor (sort-of by design), this returns if that open succeeded.
     */
    bool isOpen() const;

    /*!
     * Move the I/O pointer to \a offset in the file from position \a p.  This
     * defaults to seeking from the beginning of the file.
     *
     * \see Position
     */
    void seek(long offset, Position p = Beginning);

    /*!
     * Reset the end-of-file and error flags on the file.
     */
    void clear();

    /*!
     * Returns the current offset within the file.
     */
    long tell() const;

    /*!
     * Returns the length of the file.
     */
    long length();

    /*!
     * Truncates the file to a \a length.
     */
    void truncate(long length);

  protected:

    /*!
     * Returns the buffer size that is used for internal buffering.
     */
    static unsigned int bufferSize();

  private:
    class FileStreamPrivate;
    FileStreamPrivate *d;
  };

}

#endif
