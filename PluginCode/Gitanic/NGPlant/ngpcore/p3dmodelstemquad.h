/***************************************************************************

 Copyright (c) 2007 Sergey Prokhorchuk.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. Neither the name of the author nor the names of contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.

***************************************************************************/

#ifndef __P3DMODELSTEMQUAD_H__
#define __P3DMODELSTEMQUAD_H__

#include <ngpcore/p3dmodel.h>

#include <ngpcore/p3dmathspline.h>

class P3DStemModelQuad : public P3DStemModel
 {
  public           :

                   P3DStemModelQuad   ();

  virtual P3DStemModelInstance
                  *CreateInstance     (P3DMathRNG         *RNG,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation) const;

  virtual void     ReleaseInstance    (P3DStemModelInstance
                                                          *Instance) const;

  virtual P3DStemModel
                  *CreateCopy         () const;

  virtual bool     IsCloneable        (bool AllowScaling) const;

  /* Per-attribute information */

  virtual
  unsigned_int32     GetVAttrCount      (unsigned_int32        Attr) const;

  virtual void     FillCloneVAttrBuffer
                                      (void               *VAttrBuffer,
                                       unsigned_int32        Attr) const;

  virtual
  unsigned_int32     GetPrimitiveCount  () const;
  virtual
  unsigned_int32     GetPrimitiveType   (unsigned_int32        PrimitiveIndex) const;

  virtual void     FillVAttrIndexBuffer
                                      (void               *IndexBuffer,
                                       unsigned_int32        Attr,
                                       unsigned_int32        ElementType,
                                       unsigned_int32        IndexBase = 0) const;

  /* Per-index information */

  virtual
  unsigned_int32     GetVAttrCountI     () const;

  virtual void     FillCloneVAttrBufferI
                                      (void               *VAttrBuffer,
                                       unsigned_int32        Attr,
                                       unsigned_int32        Stride) const;

  virtual
  unsigned_int32     GetIndexCount      (unsigned_int32        PrimitiveType) const;

  virtual void     FillIndexBuffer    (void               *IndexBuffer,
                                       unsigned_int32        PrimitiveType,
                                       unsigned_int32        ElementType,
                                       unsigned_int32        IndexBase = 0) const;

  virtual void     Save               (P3DOutputStringStream
                                                          *TargetStream) const;

  virtual void     Load               (P3DInputStringFmtStream
                                                          *SourceStream,
                                       const P3DFileVersion
                                                          *Version);

  void             SetLength          (float               Length);
  float            GetLength          () const;
  void             SetWidth           (float               Width);
  float            GetWidth           () const;

  bool             IsBillboard        () const;
  unsigned_int32     GetBillboardMode   () const;
  void             SetBillboardMode   (unsigned_int32        Mode);

  void             SetScalingCurve    (const P3DMathNaturalCubicSpline
                                                          *Curve);

  const P3DMathNaturalCubicSpline
                  *GetScalingCurve    () const;

  void             SetSectionCount    (unsigned_int32        SectionCount);
  unsigned_int32     GetSectionCount    () const;

  void             SetCurvature       (const P3DMathNaturalCubicSpline
                                                          *Curve);

  const P3DMathNaturalCubicSpline
                  *GetCurvature       () const;

  void             SetThickness       (float               Thickness);
  float            GetThickness       () const;

  static void      MakeDefaultScalingCurve
                                      (P3DMathNaturalCubicSpline
                                                          &Curve);
  static void      MakeDefaultCurvatureCurve
                                      (P3DMathNaturalCubicSpline
                                                          &Curve);

  private          :

  float                                Length;
  float                                Width;

  unsigned_int32                         BillboardMode;

  P3DMathNaturalCubicSpline            ScalingCurve;

  unsigned_int32                         SectionCount;
  P3DMathNaturalCubicSpline            Curvature;
  float                                Thickness;
 };

#endif

