// -*- c-basic-offset: 4 -*-
/** @file ImageCache.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _IMAGECACHE_H
#define _IMAGECACHE_H

#include <map>
//#include <ios>
#include <wx/image.h>
#include "vigra/stdimage.hxx"
#include "common/utils.h"


// BUG: all the smartpointer/ref counting is not used currently.
//      maybe I'll fix it in the future, if I find a way how it
//      should work.

template <typename SUBJECT>
class Observer
{
public:
    virtual ~Observer()
        { }
    virtual void notify(SUBJECT & subject) = 0;
};

/** reference counting class.
 *
 *  Its sole purpose is to count references and
 *  notify a listener if the reference count
 *  goes to 1. (This indicates that only one
 *  object is in use. Usually the Observer will be the owner
 *  of that subject.
 *
 *  A Observer will be notified, the subject can
 *  be given here as well, since sometimes it is not
 *  convienet to notify about a change in CountNotifier.
 */
template <typename SUBJECT>
class RefCountNotifier
{
public:
    RefCountNotifier(Observer<SUBJECT> & observer)
        : count(0), observer(observer)
        { };

    virtual void addRef(SUBJECT &subj)
        {
            count++;
            DEBUG_DEBUG("Cnt notifier: addRef on " << std::hex << &subj << " count: " << count);
        }
    virtual void removeRef(SUBJECT &subj)
        {
            count--;
            if (count == 1) {
                observer.notify(subj);
            }
            DEBUG_DEBUG("Cnt notifier removeRef on " << std::hex << &subj << " count: " << count);
        }
    // for debugging
    int getCount()
        { return count; }
private:
    int count;
    Observer<SUBJECT> &observer;
};

/** a special kind of reference counting smart pointer.
 *
 *  This class will call a handler function
 *  if its use count goes back to one. This is particulary
 *  useful for wxImages and ImageCache. The ImageCache is
 *  notified whenever the usecount drops to 1. It can then
 *  decide to deallocate the image (or not, depending on the
 *  the free memory).
 *
 */
template <typename T, typename REFCOUNTER>
class RefCountPtr
{
public:
    /** construct a null RefCountPtr */
    RefCountPtr()
        : refCounter(0), ptr(0)
        { }

    /** construct a new NotifyPtr.
     */
    RefCountPtr(T * ptr, REFCOUNTER * refCount)
        : refCounter(refCount), ptr(ptr)
        {
            refCounter->addRef(*ptr);
        }

    /** Copy constructor.
     */
    RefCountPtr(const RefCountPtr<T, REFCOUNTER> & rhs)
        {
            DEBUG_DEBUG("RefCountPtr copy constructor for 0x"
                        << std::hex << ptr << " = 0x" << std::hex << rhs.ptr);
            if (ptr) {
                refCounter->removeRef(*ptr);
            }
            refCounter = rhs.refCounter;
            ptr = rhs.ptr;
            if (ptr) {
                refCounter->addRef(*ptr);
            }
        }
    /** destructor. Decreases reference count
     */
    ~RefCountPtr()
        {
            if (ptr) {
                refCounter->removeRef(*ptr);
            }
        }

    /** assignment operator for assigment from other smart pointers
     */
    RefCountPtr & operator=(const RefCountPtr<T, REFCOUNTER> & rhs)
        {
            if (ptr) {
                refCounter->removeRef(*ptr);
            }
            refCounter = rhs.refCounter;
            ptr = rhs.ptr;
            if (ptr) {
                refCounter->addRef(*ptr);
            }
            return *this;
        }

    /** assigment pointer for assignment from a T.
     *
     *  It can only be used to release the smart pointer (reset it to 0)
     */
    RefCountPtr & operator=(const T * rhs)
        {
            assert(rhs == 0);
            if (ptr) {
                refCounter->removeRef(&ptr);
            }
            ptr = 0;
            refCounter = 0;
        }

    /** dereference ptr */
    T * operator->() const
        { return ptr; }
    T & operator*() const
        { return *ptr; }

private:
    REFCOUNTER * refCounter;
    T *ptr;
};

//typedef RefCountPtr<wxImage, RefCountNotifier<wxImage> > ImagePtr;
typedef wxImage * ImagePtr;


/** key for an image. used to find images, and to store access information.
 *
 *  Key is misnamed, because its more than just a key.
 */
struct ImageKey
{
    /// name of the image
    std::string name;
    /// producer (for special images)
    std::string producer;
    /// number of accesses
    int accesses;

    bool operator==(const ImageKey& o) const
        { return name == o.name && producer == o.producer; }
};

/** This is a cache for all the images we use.
 *
 *  is a singleton for easy access from everywhere.
 *  The cache is used as an image source, that needs
 *  to know how to reproduce the requested images, in case
 *  that they have been deleted.
 *
 *  @todo: implement a strategy for smart deletion of images
 *
 *  @todo: possibility to store derived images as well
 *         (pyramid images etc)?
 */
class ImageCache
{
public:
    /** dtor.
     */
    virtual ~ImageCache();
    
    /** clear all images in the cache */
    void clear()
        { images.clear(); }

    /** get the global ImageCache object */
    static ImageCache & getInstance();

    /** get a image.
     *
     *  it will be loaded if its not already in the cache
     *
     *  Do not modify this image. Use a copy if it is really needed
     */
    ImagePtr getImage(const std::string & filename);

    /** get an small image version.
     *
     *  This image is 512x512 pixel maximum and can be used for icons
     *  and different previews. It is directly derived from the original.
     *
     *  @todo let selfdefined images been added belonging to the original one.
     *  @todo create substitute, remove commands
     *  @todo avoid smaller images as original
     */
    ImagePtr getImageSmall(const std::string & filename);

    /** get a pyramid image.
     *
     *  A image pyramid is a image in multiple resolutions.
     *  Usually it is used to accelerate image processing, by using
     *  lower resolutions first. they are properly low pass filtered,
     *  so no undersampling occurs (it would if one just takes
     *  every 2^level pixel instead).
     *
     *  @param filename of source image
     *  @param level of pyramid. height and width are calculated as
     *         follows: height/(level^2), width/(level^1)
     *
     */
    const vigra::BImage & getPyramidImage(const std::string & filename,
                                          int level);

private:
    /** ctor. private, nobody execpt us can create an instance.
     */
    ImageCache();

    static ImageCache * instance;

    std::map<std::string, ImagePtr> images;

    // key for your pyramid map.
    struct PyramidKey{
        PyramidKey(const std::string & str, int lv)
            : filename(str), level(lv) { }
        std::string toString()
            { return filename + utils::lexical_cast<std::string>(level); }
        std::string filename;
        int level;
    };
    std::map<std::string, vigra::BImage *> pyrImages;
};

#endif // _IMAGECACHE_H
