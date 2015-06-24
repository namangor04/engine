// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sky/engine/core/frame/ImageBitmap.h"

#include "sky/engine/core/html/ImageData.h"
#include "sky/engine/platform/graphics/BitmapImage.h"
#include "sky/engine/platform/graphics/GraphicsContext.h"
#include "sky/engine/platform/graphics/ImageBuffer.h"
#include "sky/engine/wtf/RefPtr.h"

namespace blink {

static inline IntRect normalizeRect(const IntRect& rect)
{
    return IntRect(std::min(rect.x(), rect.maxX()),
        std::min(rect.y(), rect.maxY()),
        std::max(rect.width(), -rect.width()),
        std::max(rect.height(), -rect.height()));
}

static inline PassRefPtr<Image> cropImage(Image* image, const IntRect& cropRect)
{
    IntRect intersectRect = intersection(IntRect(IntPoint(), image->size()), cropRect);
    if (!intersectRect.width() || !intersectRect.height())
        return nullptr;

    SkBitmap cropped;
    image->nativeImageForCurrentFrame()->bitmap().extractSubset(&cropped, intersectRect);
    return BitmapImage::create(NativeImageSkia::create(cropped));
}

ImageBitmap::ImageBitmap(HTMLImageElement* image, const IntRect& cropRect)
    : m_imageElement(image)
    , m_bitmap(nullptr)
    , m_cropRect(cropRect)
{
    IntRect srcRect = intersection(cropRect, IntRect(0, 0, image->width(), image->height()));
    m_bitmapRect = IntRect(IntPoint(std::max(0, -cropRect.x()), std::max(0, -cropRect.y())), srcRect.size());
    m_bitmapOffset = srcRect.location();

    if (!srcRect.width() || !srcRect.height())
        m_imageElement = nullptr;
    else
        m_imageElement->addClient(this);

}

ImageBitmap::ImageBitmap(ImageData* data, const IntRect& cropRect)
    : m_imageElement(nullptr)
    , m_cropRect(cropRect)
    , m_bitmapOffset(IntPoint())
{
    IntRect srcRect = intersection(cropRect, IntRect(IntPoint(), data->size()));

    OwnPtr<ImageBuffer> buf = ImageBuffer::create(data->size());
    if (!buf)
        return;
    if (srcRect.width() > 0 && srcRect.height() > 0)
        buf->putByteArray(Premultiplied, data->data(), data->size(), srcRect, IntPoint(std::min(0, -cropRect.x()), std::min(0, -cropRect.y())));

    m_bitmap = buf->copyImage(DontCopyBackingStore);
    m_bitmapRect = IntRect(IntPoint(std::max(0, -cropRect.x()), std::max(0, -cropRect.y())),  srcRect.size());
}

ImageBitmap::ImageBitmap(ImageBitmap* bitmap, const IntRect& cropRect)
    : m_imageElement(bitmap->imageElement())
    , m_bitmap(nullptr)
    , m_cropRect(cropRect)
    , m_bitmapOffset(IntPoint())
{
    IntRect oldBitmapRect = bitmap->bitmapRect();
    IntRect srcRect = intersection(cropRect, oldBitmapRect);
    m_bitmapRect = IntRect(IntPoint(std::max(0, oldBitmapRect.x() - cropRect.x()), std::max(0, oldBitmapRect.y() - cropRect.y())), srcRect.size());

    if (m_imageElement) {
        m_imageElement->addClient(this);
        m_bitmapOffset = srcRect.location();
    } else if (bitmap->bitmapImage()) {
        IntRect adjustedCropRect(IntPoint(cropRect.x() -oldBitmapRect.x(), cropRect.y() - oldBitmapRect.y()), cropRect.size());
        m_bitmap = cropImage(bitmap->bitmapImage().get(), adjustedCropRect);
    }
}

ImageBitmap::ImageBitmap(Image* image, const IntRect& cropRect)
    : m_imageElement(nullptr)
    , m_cropRect(cropRect)
{
    IntRect srcRect = intersection(cropRect, IntRect(IntPoint(), image->size()));
    m_bitmap = cropImage(image, cropRect);
    m_bitmapRect = IntRect(IntPoint(std::max(0, -cropRect.x()), std::max(0, -cropRect.y())),  srcRect.size());
}

ImageBitmap::~ImageBitmap()
{
#if !ENABLE(OILPAN)
    if (m_imageElement)
        m_imageElement->removeClient(this);
#endif
}

PassRefPtr<ImageBitmap> ImageBitmap::create(HTMLImageElement* image, const IntRect& cropRect)
{
    IntRect normalizedCropRect = normalizeRect(cropRect);
    return adoptRef(new ImageBitmap(image, normalizedCropRect));
}

PassRefPtr<ImageBitmap> ImageBitmap::create(ImageData* data, const IntRect& cropRect)
{
    IntRect normalizedCropRect = normalizeRect(cropRect);
    return adoptRef(new ImageBitmap(data, normalizedCropRect));
}

PassRefPtr<ImageBitmap> ImageBitmap::create(ImageBitmap* bitmap, const IntRect& cropRect)
{
    IntRect normalizedCropRect = normalizeRect(cropRect);
    return adoptRef(new ImageBitmap(bitmap, normalizedCropRect));
}

PassRefPtr<ImageBitmap> ImageBitmap::create(Image* image, const IntRect& cropRect)
{
    IntRect normalizedCropRect = normalizeRect(cropRect);
    return adoptRef(new ImageBitmap(image, normalizedCropRect));
}

void ImageBitmap::notifyImageSourceChanged()
{
    m_bitmap = cropImage(m_imageElement->cachedImage()->image(), m_cropRect);
    m_bitmapOffset = IntPoint();
    m_imageElement = nullptr;
}

PassRefPtr<Image> ImageBitmap::bitmapImage() const
{
    ASSERT((m_imageElement || m_bitmap || !m_bitmapRect.width() || !m_bitmapRect.height()) && (!m_imageElement || !m_bitmap));
    if (m_imageElement)
        return m_imageElement->cachedImage()->image();
    return m_bitmap;
}

}
