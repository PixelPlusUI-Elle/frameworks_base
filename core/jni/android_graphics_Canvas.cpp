/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jni.h"
#include "GraphicsJNI.h"
#include "core_jni_helpers.h"

#include <androidfw/ResourceTypes.h>
#include <hwui/Canvas.h>
#include <hwui/Paint.h>
#include <hwui/Typeface.h>
#include <minikin/Layout.h>

#include "Bitmap.h"
#include "SkDrawFilter.h"
#include "SkGraphics.h"

namespace android {

namespace CanvasJNI {

static Canvas* get_canvas(jlong canvasHandle) {
    return reinterpret_cast<Canvas*>(canvasHandle);
}

static void delete_canvas(Canvas* canvas) {
    delete canvas;
}

static jlong getNativeFinalizer(JNIEnv* env, jobject clazz) {
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(&delete_canvas));
}

// Native wrapper constructor used by Canvas(Bitmap)
static jlong initRaster(JNIEnv* env, jobject, jobject jbitmap) {
    SkBitmap bitmap;
    if (jbitmap != NULL) {
        GraphicsJNI::getSkBitmap(env, jbitmap, &bitmap);
    }
    return reinterpret_cast<jlong>(Canvas::create_canvas(bitmap));
}

// Set the given bitmap as the new draw target (wrapped in a new SkCanvas),
// optionally copying canvas matrix & clip state.
static void setBitmap(JNIEnv* env, jobject, jlong canvasHandle, jobject jbitmap) {
    SkBitmap bitmap;
    if (jbitmap != NULL) {
        GraphicsJNI::getSkBitmap(env, jbitmap, &bitmap);
    }
    get_canvas(canvasHandle)->setBitmap(bitmap);
}

static jboolean isOpaque(JNIEnv*, jobject, jlong canvasHandle) {
    return get_canvas(canvasHandle)->isOpaque() ? JNI_TRUE : JNI_FALSE;
}

static jint getWidth(JNIEnv*, jobject, jlong canvasHandle) {
    return static_cast<jint>(get_canvas(canvasHandle)->width());
}

static jint getHeight(JNIEnv*, jobject, jlong canvasHandle) {
    return static_cast<jint>(get_canvas(canvasHandle)->height());
}

static void setHighContrastText(JNIEnv*, jobject, jlong canvasHandle, jboolean highContrastText) {
    Canvas* canvas = get_canvas(canvasHandle);
    canvas->setHighContrastText(highContrastText);
}

static jint getSaveCount(JNIEnv*, jobject, jlong canvasHandle) {
    return static_cast<jint>(get_canvas(canvasHandle)->getSaveCount());
}

static jint save(JNIEnv*, jobject, jlong canvasHandle, jint flagsHandle) {
    SaveFlags::Flags flags = static_cast<SaveFlags::Flags>(flagsHandle);
    return static_cast<jint>(get_canvas(canvasHandle)->save(flags));
}

static jint saveLayer(JNIEnv* env, jobject, jlong canvasHandle, jfloat l, jfloat t,
                      jfloat r, jfloat b, jlong paintHandle, jint flagsHandle) {
    Paint* paint  = reinterpret_cast<Paint*>(paintHandle);
    SaveFlags::Flags flags = static_cast<SaveFlags::Flags>(flagsHandle);
    return static_cast<jint>(get_canvas(canvasHandle)->saveLayer(l, t, r, b, paint, flags));
}

static jint saveLayerAlpha(JNIEnv* env, jobject, jlong canvasHandle, jfloat l, jfloat t,
                           jfloat r, jfloat b, jint alpha, jint flagsHandle) {
    SaveFlags::Flags flags = static_cast<SaveFlags::Flags>(flagsHandle);
    return static_cast<jint>(get_canvas(canvasHandle)->saveLayerAlpha(l, t, r, b, alpha, flags));
}

static void restore(JNIEnv* env, jobject, jlong canvasHandle, jboolean throwOnUnderflow) {
    Canvas* canvas = get_canvas(canvasHandle);
    if (canvas->getSaveCount() <= 1) {  // cannot restore anymore
        if (throwOnUnderflow) {
            doThrowISE(env, "Underflow in restore - more restores than saves");
        }
        return; // compat behavior - return without throwing
    }
    canvas->restore();
}

static void restoreToCount(JNIEnv* env, jobject, jlong canvasHandle, jint restoreCount,
        jboolean throwOnUnderflow) {
    Canvas* canvas = get_canvas(canvasHandle);
    if (restoreCount < 1 || restoreCount > canvas->getSaveCount()) {
        if (throwOnUnderflow) {
            doThrowIAE(env, "Underflow in restoreToCount - more restores than saves");
            return;
        }
        restoreCount = 1; // compat behavior - restore as far as possible
    }
    canvas->restoreToCount(restoreCount);
}

static void getCTM(JNIEnv* env, jobject, jlong canvasHandle, jlong matrixHandle) {
    SkMatrix* matrix = reinterpret_cast<SkMatrix*>(matrixHandle);
    get_canvas(canvasHandle)->getMatrix(matrix);
}

static void setMatrix(JNIEnv* env, jobject, jlong canvasHandle, jlong matrixHandle) {
    const SkMatrix* matrix = reinterpret_cast<SkMatrix*>(matrixHandle);
    get_canvas(canvasHandle)->setMatrix(matrix ? *matrix : SkMatrix::I());
}

static void concat(JNIEnv* env, jobject, jlong canvasHandle, jlong matrixHandle) {
    const SkMatrix* matrix = reinterpret_cast<SkMatrix*>(matrixHandle);
    get_canvas(canvasHandle)->concat(*matrix);
}

static void rotate(JNIEnv*, jobject, jlong canvasHandle, jfloat degrees) {
    get_canvas(canvasHandle)->rotate(degrees);
}

static void scale(JNIEnv*, jobject, jlong canvasHandle, jfloat sx, jfloat sy) {
    get_canvas(canvasHandle)->scale(sx, sy);
}

static void skew(JNIEnv*, jobject, jlong canvasHandle, jfloat sx, jfloat sy) {
    get_canvas(canvasHandle)->skew(sx, sy);
}

static void translate(JNIEnv*, jobject, jlong canvasHandle, jfloat dx, jfloat dy) {
    get_canvas(canvasHandle)->translate(dx, dy);
}

static jboolean getClipBounds(JNIEnv* env, jobject, jlong canvasHandle, jobject bounds) {
    SkRect   r;
    SkIRect ir;
    bool result = get_canvas(canvasHandle)->getClipBounds(&r);

    if (!result) {
        r.setEmpty();
    }
    r.round(&ir);

    (void)GraphicsJNI::irect_to_jrect(ir, env, bounds);
    return result ? JNI_TRUE : JNI_FALSE;
}

static jboolean quickRejectRect(JNIEnv* env, jobject, jlong canvasHandle,
                                jfloat left, jfloat top, jfloat right, jfloat bottom) {
    bool result = get_canvas(canvasHandle)->quickRejectRect(left, top, right, bottom);
    return result ? JNI_TRUE : JNI_FALSE;
}

static jboolean quickRejectPath(JNIEnv* env, jobject, jlong canvasHandle, jlong pathHandle) {
    SkPath* path = reinterpret_cast<SkPath*>(pathHandle);
    bool result = get_canvas(canvasHandle)->quickRejectPath(*path);
    return result ? JNI_TRUE : JNI_FALSE;
}

static jboolean clipRect(JNIEnv*, jobject, jlong canvasHandle, jfloat l, jfloat t,
                         jfloat r, jfloat b, jint opHandle) {
    SkRegion::Op op = static_cast<SkRegion::Op>(opHandle);
    bool nonEmptyClip = get_canvas(canvasHandle)->clipRect(l, t, r, b, op);
    return nonEmptyClip ? JNI_TRUE : JNI_FALSE;
}

static jboolean clipPath(JNIEnv* env, jobject, jlong canvasHandle, jlong pathHandle,
                         jint opHandle) {
    SkPath* path = reinterpret_cast<SkPath*>(pathHandle);
    SkRegion::Op op = static_cast<SkRegion::Op>(opHandle);
    bool nonEmptyClip = get_canvas(canvasHandle)->clipPath(path, op);
    return nonEmptyClip ? JNI_TRUE : JNI_FALSE;
}

static jboolean clipRegion(JNIEnv* env, jobject, jlong canvasHandle, jlong deviceRgnHandle,
                           jint opHandle) {
    SkRegion* deviceRgn = reinterpret_cast<SkRegion*>(deviceRgnHandle);
    SkRegion::Op op = static_cast<SkRegion::Op>(opHandle);
    bool nonEmptyClip = get_canvas(canvasHandle)->clipRegion(deviceRgn, op);
    return nonEmptyClip ? JNI_TRUE : JNI_FALSE;
}

static void drawColor(JNIEnv* env, jobject, jlong canvasHandle, jint color, jint modeHandle) {
    SkBlendMode mode = static_cast<SkBlendMode>(modeHandle);
    get_canvas(canvasHandle)->drawColor(color, mode);
}

static void drawPaint(JNIEnv* env, jobject, jlong canvasHandle, jlong paintHandle) {
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawPaint(*paint);
}

static void drawPoint(JNIEnv*, jobject, jlong canvasHandle, jfloat x, jfloat y,
                      jlong paintHandle) {
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawPoint(x, y, *paint);
}

static void drawPoints(JNIEnv* env, jobject, jlong canvasHandle, jfloatArray jptsArray,
                       jint offset, jint count, jlong paintHandle) {
    NPE_CHECK_RETURN_VOID(env, jptsArray);
    AutoJavaFloatArray autoPts(env, jptsArray);
    float* floats = autoPts.ptr();
    const int length = autoPts.length();

    if ((offset | count) < 0 || offset + count > length) {
        doThrowAIOOBE(env);
        return;
    }

    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawPoints(floats + offset, count, *paint);
}

static void drawLine(JNIEnv* env, jobject, jlong canvasHandle, jfloat startX, jfloat startY,
                     jfloat stopX, jfloat stopY, jlong paintHandle) {
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawLine(startX, startY, stopX, stopY, *paint);
}

static void drawLines(JNIEnv* env, jobject, jlong canvasHandle, jfloatArray jptsArray,
                      jint offset, jint count, jlong paintHandle) {
    NPE_CHECK_RETURN_VOID(env, jptsArray);
    AutoJavaFloatArray autoPts(env, jptsArray);
    float* floats = autoPts.ptr();
    const int length = autoPts.length();

    if ((offset | count) < 0 || offset + count > length) {
        doThrowAIOOBE(env);
        return;
    }

    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawLines(floats + offset, count, *paint);
}

static void drawRect(JNIEnv* env, jobject, jlong canvasHandle, jfloat left, jfloat top,
                     jfloat right, jfloat bottom, jlong paintHandle) {
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawRect(left, top, right, bottom, *paint);
}

static void drawRegion(JNIEnv* env, jobject, jlong canvasHandle, jlong regionHandle,
                       jlong paintHandle) {
    const SkRegion* region = reinterpret_cast<SkRegion*>(regionHandle);
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawRegion(*region, *paint);
}

static void drawRoundRect(JNIEnv* env, jobject, jlong canvasHandle, jfloat left, jfloat top,
                          jfloat right, jfloat bottom, jfloat rx, jfloat ry, jlong paintHandle) {
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawRoundRect(left, top, right, bottom, rx, ry, *paint);
}

static void drawCircle(JNIEnv* env, jobject, jlong canvasHandle, jfloat cx, jfloat cy,
                       jfloat radius, jlong paintHandle) {
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawCircle(cx, cy, radius, *paint);
}

static void drawOval(JNIEnv* env, jobject, jlong canvasHandle, jfloat left, jfloat top,
                     jfloat right, jfloat bottom, jlong paintHandle) {
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawOval(left, top, right, bottom, *paint);
}

static void drawArc(JNIEnv* env, jobject, jlong canvasHandle, jfloat left, jfloat top,
                    jfloat right, jfloat bottom, jfloat startAngle, jfloat sweepAngle,
                    jboolean useCenter, jlong paintHandle) {
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawArc(left, top, right, bottom, startAngle, sweepAngle,
                                       useCenter, *paint);
}

static void drawPath(JNIEnv* env, jobject, jlong canvasHandle, jlong pathHandle,
                     jlong paintHandle) {
    const SkPath* path = reinterpret_cast<SkPath*>(pathHandle);
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawPath(*path, *paint);
}

static void drawVertices(JNIEnv* env, jobject, jlong canvasHandle,
                         jint modeHandle, jint vertexCount,
                         jfloatArray jverts, jint vertIndex,
                         jfloatArray jtexs, jint texIndex,
                         jintArray jcolors, jint colorIndex,
                         jshortArray jindices, jint indexIndex,
                         jint indexCount, jlong paintHandle) {
    AutoJavaFloatArray  vertA(env, jverts, vertIndex + vertexCount);
    AutoJavaFloatArray  texA(env, jtexs, texIndex + vertexCount);
    AutoJavaIntArray    colorA(env, jcolors, colorIndex + vertexCount);
    AutoJavaShortArray  indexA(env, jindices, indexIndex + indexCount);

    const float* verts = vertA.ptr() + vertIndex;
    const float* texs = texA.ptr() + vertIndex;
    const int* colors = NULL;
    const uint16_t* indices = NULL;

    if (jcolors != NULL) {
        colors = colorA.ptr() + colorIndex;
    }
    if (jindices != NULL) {
        indices = (const uint16_t*)(indexA.ptr() + indexIndex);
    }

    SkCanvas::VertexMode mode = static_cast<SkCanvas::VertexMode>(modeHandle);
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawVertices(mode, vertexCount, verts, texs, colors,
                                           indices, indexCount, *paint);
}

static void drawNinePatch(JNIEnv* env, jobject, jlong canvasHandle, jlong bitmapHandle,
        jlong chunkHandle, jfloat left, jfloat top, jfloat right, jfloat bottom,
        jlong paintHandle, jint dstDensity, jint srcDensity) {

    Canvas* canvas = get_canvas(canvasHandle);
    SkBitmap skiaBitmap;
    bitmap::toSkBitmap(bitmapHandle, &skiaBitmap);
    const android::Res_png_9patch* chunk = reinterpret_cast<android::Res_png_9patch*>(chunkHandle);
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);

    if (CC_LIKELY(dstDensity == srcDensity || dstDensity == 0 || srcDensity == 0)) {
        canvas->drawNinePatch(skiaBitmap, *chunk, left, top, right, bottom, paint);
    } else {
        canvas->save(SaveFlags::MatrixClip);

        SkScalar scale = dstDensity / (float)srcDensity;
        canvas->translate(left, top);
        canvas->scale(scale, scale);

        Paint filteredPaint;
        if (paint) {
            filteredPaint = *paint;
        }
        filteredPaint.setFilterQuality(kLow_SkFilterQuality);

        canvas->drawNinePatch(skiaBitmap, *chunk, 0, 0, (right-left)/scale, (bottom-top)/scale,
                &filteredPaint);

        canvas->restore();
    }
}

static void drawBitmap(JNIEnv* env, jobject, jlong canvasHandle, jobject jbitmap,
                       jfloat left, jfloat top, jlong paintHandle, jint canvasDensity,
                       jint screenDensity, jint bitmapDensity) {
    Canvas* canvas = get_canvas(canvasHandle);
    Bitmap& bitmap = android::bitmap::toBitmap(env, jbitmap);
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);

    if (canvasDensity == bitmapDensity || canvasDensity == 0 || bitmapDensity == 0) {
        if (screenDensity != 0 && screenDensity != bitmapDensity) {
            Paint filteredPaint;
            if (paint) {
                filteredPaint = *paint;
            }
            filteredPaint.setFilterQuality(kLow_SkFilterQuality);
            canvas->drawBitmap(bitmap, left, top, &filteredPaint);
        } else {
            canvas->drawBitmap(bitmap, left, top, paint);
        }
    } else {
        canvas->save(SaveFlags::MatrixClip);
        SkScalar scale = canvasDensity / (float)bitmapDensity;
        canvas->translate(left, top);
        canvas->scale(scale, scale);

        Paint filteredPaint;
        if (paint) {
            filteredPaint = *paint;
        }
        filteredPaint.setFilterQuality(kLow_SkFilterQuality);

        canvas->drawBitmap(bitmap, 0, 0, &filteredPaint);
        canvas->restore();
    }
}

static void drawBitmapMatrix(JNIEnv* env, jobject, jlong canvasHandle, jobject jbitmap,
                             jlong matrixHandle, jlong paintHandle) {
    const SkMatrix* matrix = reinterpret_cast<SkMatrix*>(matrixHandle);
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    SkBitmap bitmap;
    GraphicsJNI::getSkBitmap(env, jbitmap, &bitmap);
    get_canvas(canvasHandle)->drawBitmap(bitmap, *matrix, paint);
}

static void drawBitmapRect(JNIEnv* env, jobject, jlong canvasHandle, jobject jbitmap,
                           float srcLeft, float srcTop, float srcRight, float srcBottom,
                           float dstLeft, float dstTop, float dstRight, float dstBottom,
                           jlong paintHandle, jint screenDensity, jint bitmapDensity) {
    Canvas* canvas = get_canvas(canvasHandle);
    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);

    SkBitmap bitmap;
    GraphicsJNI::getSkBitmap(env, jbitmap, &bitmap);
    if (screenDensity != 0 && screenDensity != bitmapDensity) {
        Paint filteredPaint;
        if (paint) {
            filteredPaint = *paint;
        }
        filteredPaint.setFilterQuality(kLow_SkFilterQuality);
        canvas->drawBitmap(bitmap, srcLeft, srcTop, srcRight, srcBottom,
                           dstLeft, dstTop, dstRight, dstBottom, &filteredPaint);
    } else {
        canvas->drawBitmap(bitmap, srcLeft, srcTop, srcRight, srcBottom,
                           dstLeft, dstTop, dstRight, dstBottom, paint);
    }
}

static void drawBitmapArray(JNIEnv* env, jobject, jlong canvasHandle,
                            jintArray jcolors, jint offset, jint stride,
                            jfloat x, jfloat y, jint width, jint height,
                            jboolean hasAlpha, jlong paintHandle) {
    // Note: If hasAlpha is false, kRGB_565_SkColorType will be used, which will
    // correct the alphaType to kOpaque_SkAlphaType.
    SkImageInfo info = SkImageInfo::MakeN32(width, height, kPremul_SkAlphaType,
            GraphicsJNI::defaultColorSpace());
    SkBitmap bitmap;
    bitmap.setInfo(info);
    sk_sp<Bitmap> androidBitmap = Bitmap::allocateHeapBitmap(&bitmap, NULL);
    if (!androidBitmap) {
        return;
    }

    if (!GraphicsJNI::SetPixels(env, jcolors, offset, stride, 0, 0, width, height, bitmap)) {
        return;
    }

    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    get_canvas(canvasHandle)->drawBitmap(*androidBitmap, x, y, paint);
}

static void drawBitmapMesh(JNIEnv* env, jobject, jlong canvasHandle, jobject jbitmap,
                           jint meshWidth, jint meshHeight, jfloatArray jverts,
                           jint vertIndex, jintArray jcolors, jint colorIndex, jlong paintHandle) {
    const int ptCount = (meshWidth + 1) * (meshHeight + 1);
    AutoJavaFloatArray vertA(env, jverts, vertIndex + (ptCount << 1));
    AutoJavaIntArray colorA(env, jcolors, colorIndex + ptCount);

    const Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    SkBitmap bitmap;
    GraphicsJNI::getSkBitmap(env, jbitmap, &bitmap);
    get_canvas(canvasHandle)->drawBitmapMesh(bitmap, meshWidth, meshHeight,
                                             vertA.ptr(), colorA.ptr(), paint);
}

static void drawTextChars(JNIEnv* env, jobject, jlong canvasHandle, jcharArray text,
                          jint index, jint count, jfloat x, jfloat y, jint bidiFlags,
                          jlong paintHandle, jlong typefaceHandle) {
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    Typeface* typeface = reinterpret_cast<Typeface*>(typefaceHandle);
    jchar* jchars = env->GetCharArrayElements(text, NULL);
    get_canvas(canvasHandle)->drawText(jchars + index, 0, count, count, x, y,
                                       bidiFlags, *paint, typeface);
    env->ReleaseCharArrayElements(text, jchars, JNI_ABORT);
}

static void drawTextString(JNIEnv* env, jobject, jlong canvasHandle, jstring text,
                           jint start, jint end, jfloat x, jfloat y, jint bidiFlags,
                           jlong paintHandle, jlong typefaceHandle) {
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    Typeface* typeface = reinterpret_cast<Typeface*>(typefaceHandle);
    const int count = end - start;
    const jchar* jchars = env->GetStringChars(text, NULL);
    get_canvas(canvasHandle)->drawText(jchars + start, 0, count, count, x, y,
                                       bidiFlags, *paint, typeface);
    env->ReleaseStringChars(text, jchars);
}

static void drawTextRunChars(JNIEnv* env, jobject, jlong canvasHandle, jcharArray text, jint index,
                             jint count, jint contextIndex, jint contextCount, jfloat x, jfloat y,
                             jboolean isRtl, jlong paintHandle, jlong typefaceHandle) {
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    Typeface* typeface = reinterpret_cast<Typeface*>(typefaceHandle);

    const int bidiFlags = isRtl ? minikin::kBidi_Force_RTL : minikin::kBidi_Force_LTR;
    jchar* jchars = env->GetCharArrayElements(text, NULL);
    get_canvas(canvasHandle)->drawText(jchars + contextIndex, index - contextIndex, count,
                                       contextCount, x, y, bidiFlags, *paint, typeface);
    env->ReleaseCharArrayElements(text, jchars, JNI_ABORT);
}

static void drawTextRunString(JNIEnv* env, jobject obj, jlong canvasHandle, jstring text,
                              jint start, jint end, jint contextStart, jint contextEnd,
                              jfloat x, jfloat y, jboolean isRtl, jlong paintHandle,
                              jlong typefaceHandle) {
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    Typeface* typeface = reinterpret_cast<Typeface*>(typefaceHandle);

    int bidiFlags = isRtl ? minikin::kBidi_Force_RTL : minikin::kBidi_Force_LTR;
    jint count = end - start;
    jint contextCount = contextEnd - contextStart;
    const jchar* jchars = env->GetStringChars(text, NULL);
    get_canvas(canvasHandle)->drawText(jchars + contextStart, start - contextStart, count,
                                       contextCount, x, y, bidiFlags, *paint, typeface);
    env->ReleaseStringChars(text, jchars);
}

static void drawTextOnPathChars(JNIEnv* env, jobject, jlong canvasHandle, jcharArray text,
                                jint index, jint count, jlong pathHandle, jfloat hOffset,
                                jfloat vOffset, jint bidiFlags, jlong paintHandle,
                                jlong typefaceHandle) {
    SkPath* path = reinterpret_cast<SkPath*>(pathHandle);
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    Typeface* typeface = reinterpret_cast<Typeface*>(typefaceHandle);

    jchar* jchars = env->GetCharArrayElements(text, NULL);

    get_canvas(canvasHandle)->drawTextOnPath(jchars + index, count, bidiFlags, *path,
                   hOffset, vOffset, *paint, typeface);

    env->ReleaseCharArrayElements(text, jchars, 0);
}

static void drawTextOnPathString(JNIEnv* env, jobject, jlong canvasHandle, jstring text,
                                 jlong pathHandle, jfloat hOffset, jfloat vOffset,
                                 jint bidiFlags, jlong paintHandle, jlong typefaceHandle) {
    SkPath* path = reinterpret_cast<SkPath*>(pathHandle);
    Paint* paint = reinterpret_cast<Paint*>(paintHandle);
    Typeface* typeface = reinterpret_cast<Typeface*>(typefaceHandle);

    const jchar* jchars = env->GetStringChars(text, NULL);
    int count = env->GetStringLength(text);

    get_canvas(canvasHandle)->drawTextOnPath(jchars, count, bidiFlags, *path,
                   hOffset, vOffset, *paint, typeface);

    env->ReleaseStringChars(text, jchars);
}

static void setDrawFilter(JNIEnv* env, jobject, jlong canvasHandle, jlong filterHandle) {
    get_canvas(canvasHandle)->setDrawFilter(reinterpret_cast<SkDrawFilter*>(filterHandle));
}

static void freeCaches(JNIEnv* env, jobject) {
    SkGraphics::PurgeFontCache();
}

static void freeTextLayoutCaches(JNIEnv* env, jobject) {
    minikin::Layout::purgeCaches();
}

}; // namespace CanvasJNI

static const JNINativeMethod gMethods[] = {
    {"nGetNativeFinalizer", "()J", (void*) CanvasJNI::getNativeFinalizer},
    {"nInitRaster", "(Landroid/graphics/Bitmap;)J", (void*) CanvasJNI::initRaster},
    {"nFreeCaches", "()V", (void*) CanvasJNI::freeCaches},
    {"nFreeTextLayoutCaches", "()V", (void*) CanvasJNI::freeTextLayoutCaches},

    // ------------ @FastNative ----------------
    {"nSetBitmap", "(JLandroid/graphics/Bitmap;)V", (void*) CanvasJNI::setBitmap},
    {"nIsOpaque","(J)Z", (void*) CanvasJNI::isOpaque},
    {"nGetWidth","(J)I", (void*) CanvasJNI::getWidth},
    {"nGetHeight","(J)I", (void*) CanvasJNI::getHeight},
    {"nSetHighContrastText","(JZ)V", (void*) CanvasJNI::setHighContrastText},
    {"nSave","(JI)I", (void*) CanvasJNI::save},
    {"nSaveLayer","(JFFFFJI)I", (void*) CanvasJNI::saveLayer},
    {"nSaveLayerAlpha","(JFFFFII)I", (void*) CanvasJNI::saveLayerAlpha},
    {"nGetSaveCount","(J)I", (void*) CanvasJNI::getSaveCount},
    {"nRestore","(JZ)V", (void*) CanvasJNI::restore},
    {"nRestoreToCount","(JIZ)V", (void*) CanvasJNI::restoreToCount},
    {"nGetCTM", "(JJ)V", (void*)CanvasJNI::getCTM},
    {"nSetMatrix","(JJ)V", (void*) CanvasJNI::setMatrix},
    {"nConcat","(JJ)V", (void*) CanvasJNI::concat},
    {"nRotate","(JF)V", (void*) CanvasJNI::rotate},
    {"nScale","(JFF)V", (void*) CanvasJNI::scale},
    {"nSkew","(JFF)V", (void*) CanvasJNI::skew},
    {"nTranslate","(JFF)V", (void*) CanvasJNI::translate},
    {"nGetClipBounds","(JLandroid/graphics/Rect;)Z", (void*) CanvasJNI::getClipBounds},
    {"nQuickReject","(JJ)Z", (void*) CanvasJNI::quickRejectPath},
    {"nQuickReject","(JFFFF)Z", (void*)CanvasJNI::quickRejectRect},
    {"nClipRect","(JFFFFI)Z", (void*) CanvasJNI::clipRect},
    {"nClipPath","(JJI)Z", (void*) CanvasJNI::clipPath},
    {"nClipRegion","(JJI)Z", (void*) CanvasJNI::clipRegion},
    {"nSetDrawFilter", "(JJ)V", (void*) CanvasJNI::setDrawFilter},
};

// If called from Canvas these are regular JNI
// If called from DisplayListCanvas they are @FastNative
static const JNINativeMethod gDrawMethods[] = {
    {"nDrawColor","(JII)V", (void*) CanvasJNI::drawColor},
    {"nDrawPaint","(JJ)V", (void*) CanvasJNI::drawPaint},
    {"nDrawPoint", "(JFFJ)V", (void*) CanvasJNI::drawPoint},
    {"nDrawPoints", "(J[FIIJ)V", (void*) CanvasJNI::drawPoints},
    {"nDrawLine", "(JFFFFJ)V", (void*) CanvasJNI::drawLine},
    {"nDrawLines", "(J[FIIJ)V", (void*) CanvasJNI::drawLines},
    {"nDrawRect","(JFFFFJ)V", (void*) CanvasJNI::drawRect},
    {"nDrawRegion", "(JJJ)V", (void*) CanvasJNI::drawRegion },
    {"nDrawRoundRect","(JFFFFFFJ)V", (void*) CanvasJNI::drawRoundRect},
    {"nDrawCircle","(JFFFJ)V", (void*) CanvasJNI::drawCircle},
    {"nDrawOval","(JFFFFJ)V", (void*) CanvasJNI::drawOval},
    {"nDrawArc","(JFFFFFFZJ)V", (void*) CanvasJNI::drawArc},
    {"nDrawPath","(JJJ)V", (void*) CanvasJNI::drawPath},
    {"nDrawVertices", "(JII[FI[FI[II[SIIJ)V", (void*)CanvasJNI::drawVertices},
    {"nDrawNinePatch", "(JJJFFFFJII)V", (void*)CanvasJNI::drawNinePatch},
    {"nDrawBitmapMatrix", "(JLandroid/graphics/Bitmap;JJ)V", (void*)CanvasJNI::drawBitmapMatrix},
    {"nDrawBitmapMesh", "(JLandroid/graphics/Bitmap;II[FI[IIJ)V", (void*)CanvasJNI::drawBitmapMesh},
    {"nDrawBitmap","(JLandroid/graphics/Bitmap;FFJIII)V", (void*) CanvasJNI::drawBitmap},
    {"nDrawBitmap","(JLandroid/graphics/Bitmap;FFFFFFFFJII)V", (void*) CanvasJNI::drawBitmapRect},
    {"nDrawBitmap", "(J[IIIFFIIZJ)V", (void*)CanvasJNI::drawBitmapArray},
    {"nDrawText","(J[CIIFFIJJ)V", (void*) CanvasJNI::drawTextChars},
    {"nDrawText","(JLjava/lang/String;IIFFIJJ)V", (void*) CanvasJNI::drawTextString},
    {"nDrawTextRun","(J[CIIIIFFZJJ)V", (void*) CanvasJNI::drawTextRunChars},
    {"nDrawTextRun","(JLjava/lang/String;IIIIFFZJJ)V", (void*) CanvasJNI::drawTextRunString},
    {"nDrawTextOnPath","(J[CIIJFFIJJ)V", (void*) CanvasJNI::drawTextOnPathChars},
    {"nDrawTextOnPath","(JLjava/lang/String;JFFIJJ)V", (void*) CanvasJNI::drawTextOnPathString},
};

int register_android_graphics_Canvas(JNIEnv* env) {
    int ret = 0;
    ret |= RegisterMethodsOrDie(env, "android/graphics/Canvas", gMethods, NELEM(gMethods));
    ret |= RegisterMethodsOrDie(env, "android/graphics/BaseCanvas", gDrawMethods, NELEM(gDrawMethods));
    ret |= RegisterMethodsOrDie(env, "android/view/RecordingCanvas", gDrawMethods, NELEM(gDrawMethods));
    return ret;

}

}; // namespace android
