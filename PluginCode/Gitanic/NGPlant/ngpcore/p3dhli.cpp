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


#include <ngpcore/p3dmodel.h>
#include <ngpcore/p3dmodelstemquad.h>
#include <ngpcore/p3dhli.h>

/* calculate total group count (including plant base group) */
static
unsigned_int32       CalcInternalGroupCount
                                      (const P3DBranchModel
                                                          *BranchModel)
 {
  unsigned_int32                         GroupCount;
  unsigned_int32                         SubBranchIndex;
  unsigned_int32                         SubBranchCount;

  GroupCount = 1;

  SubBranchCount = BranchModel->GetSubBranchCount();

  for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
   {
    GroupCount += CalcInternalGroupCount(BranchModel->GetSubBranchModel(SubBranchIndex));
   }

  return(GroupCount);
 }

class P3DHLIMaterial : public P3DMaterialInstance
 {
  public           :

                   P3DHLIMaterial     (const P3DMaterialDef
                                                          &MaterialDef)
                   : MatDef(MaterialDef)
   {
   }

  virtual
  const
  P3DMaterialDef  *GetMaterialDef     () const
   {
    return(&MatDef);
   }

  virtual
  P3DMaterialInstance
                  *CreateCopy         () const
   {
    return(new P3DHLIMaterial(MatDef));
   }

  private          :

  P3DMaterialDef                       MatDef;
 };

class P3DHLIMatFactory : public P3DMaterialFactory
 {
  public           :

                   P3DHLIMatFactory   () {}

  virtual P3DMaterialInstance
                  *CreateMaterial     (const P3DMaterialDef
                                                          &MaterialDef) const
   {
    return(new P3DHLIMaterial(MaterialDef));
   }
 };

class P3DHLIBranchCalculator : public P3DBranchingFactory
 {
  public           :

                   P3DHLIBranchCalculator
                                      (P3DMathRNG         *RNG,
                                       const P3DBranchModel
                                                          *BranchModel,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       const P3DBranchModel
                                                          *CountedBranch,
                                       unsigned_int32       *Counter)
   {
    this->RNG           = RNG;
    this->BranchModel   = BranchModel;
    this->Parent        = Parent;
    this->CountedBranch = CountedBranch;
    this->Counter       = Counter;
   }

  virtual void     GenerateBranch     (float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation)
   {
    if (BranchModel == CountedBranch)
     {
      (*Counter)++;
     }

    const P3DStemModel              *StemModel;
    P3DStemModelInstance            *Instance;
    unsigned_int32                     SubBranchIndex;
    unsigned_int32                     SubBranchCount;

    StemModel = BranchModel->GetStemModel();

    if (StemModel != 0)
     {
      Instance = StemModel->CreateInstance(RNG,Parent,Offset,Orientation);
     }
    else
     {
      Instance = 0;
     }

    SubBranchCount = BranchModel->GetSubBranchCount();

    for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
     {
      P3DHLIBranchCalculator         Calculator(RNG,
                                                BranchModel->GetSubBranchModel(SubBranchIndex),
                                                Instance,
                                                CountedBranch,
                                                Counter);

      const_cast<P3DBranchingAlg*>(BranchModel->GetSubBranchModel(SubBranchIndex)->GetBranchingAlg())
       ->CreateBranches(&Calculator,Instance,RNG);
     }

    if (StemModel != 0)
     {
      StemModel->ReleaseInstance(Instance);
     }
   }

  private          :

  P3DMathRNG                          *RNG;
  const P3DBranchModel                *BranchModel;
  const P3DStemModelInstance          *Parent;
  const P3DBranchModel                *CountedBranch;
  unsigned_int32                        *Counter;
 };

class P3DHLIBranchCalculatorMulti : public P3DBranchingFactory
 {
  public           :

                   P3DHLIBranchCalculatorMulti
                                      (P3DMathRNG         *RNG,
                                       const P3DBranchModel
                                                          *BranchModel,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       unsigned_int32        GroupIndex,
                                       unsigned_int32       *Counters)
   {
    this->RNG           = RNG;
    this->BranchModel   = BranchModel;
    this->Parent        = Parent;
    this->GroupIndex    = GroupIndex;
    this->Counters      = Counters;
   }

  virtual void     GenerateBranch     (float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation)
   {
    const P3DStemModel              *StemModel;
    P3DStemModelInstance            *Instance;
    unsigned_int32                     SubBranchIndex;
    unsigned_int32                     SubBranchCount;
    unsigned_int32                     SubGroupIndex;

    StemModel = BranchModel->GetStemModel();

    if (StemModel != 0)
     {
      Instance      = StemModel->CreateInstance(RNG,Parent,Offset,Orientation);
      SubGroupIndex = GroupIndex + 1;

      Counters[GroupIndex]++;
     }
    else
     {
      Instance      = 0;
      SubGroupIndex = 0;
     }

    SubBranchCount = BranchModel->GetSubBranchCount();

    for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
     {
      P3DHLIBranchCalculatorMulti Calculator(RNG,
                                             BranchModel->GetSubBranchModel(SubBranchIndex),
                                             Instance,
                                             SubGroupIndex,
                                             Counters);

      const_cast<P3DBranchingAlg*>(BranchModel->GetSubBranchModel(SubBranchIndex)->GetBranchingAlg())
       ->CreateBranches(&Calculator,Instance,RNG);

      SubGroupIndex += CalcInternalGroupCount
                        (BranchModel->GetSubBranchModel(SubBranchIndex));
     }

    if (Instance != 0)
     {
      StemModel->ReleaseInstance(Instance);
     }
   }

  private          :

  P3DMathRNG                          *RNG;
  const P3DBranchModel                *BranchModel;
  const P3DStemModelInstance          *Parent;
  unsigned_int32                         GroupIndex;
  unsigned_int32                        *Counters;
 };

class P3DHLIFillCloneTransformBufferHelper : public P3DBranchingFactory
 {
  public           :

                   P3DHLIFillCloneTransformBufferHelper
                                      (P3DMathRNG         *RNG,
                                       const P3DBranchModel
                                                          *BranchModel,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       const P3DBranchModel
                                                          *RequiredBranch,
                                       float             **OffsetBuffer,
                                       float             **OrientationBuffer,
                                       float             **ScaleBuffer)
   {
    this->RNG               = RNG;
    this->BranchModel       = BranchModel;
    this->Parent            = Parent;
    this->RequiredBranch    = RequiredBranch;
    this->OffsetBuffer      = OffsetBuffer;
    this->OrientationBuffer = OrientationBuffer;
    this->ScaleBuffer       = ScaleBuffer;
   }

  virtual void     GenerateBranch     (float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation)
   {
    const P3DStemModel                *StemModel;
    P3DStemModelInstance              *Instance;

    StemModel = BranchModel->GetStemModel();

    if (StemModel != 0)
     {
      Instance = StemModel->CreateInstance(RNG,Parent,Offset,Orientation);
     }
    else
     {
      Instance = 0;
     }

    if (BranchModel == RequiredBranch)
     {
      P3DMatrix4x4f  m;
      P3DQuaternionf q;

      Instance->GetWorldTransform(m.m);

      if (OrientationBuffer != 0)
       {
        q.FromMatrix(m.m);

        (*OrientationBuffer)[0] = q.q[0];
        (*OrientationBuffer)[1] = q.q[1];
        (*OrientationBuffer)[2] = q.q[2];
        (*OrientationBuffer)[3] = q.q[3];

        *OrientationBuffer += 4;
       }

      if (OffsetBuffer != 0)
       {
        (*OffsetBuffer)[0] = m.m[12];
        (*OffsetBuffer)[1] = m.m[13];
        (*OffsetBuffer)[2] = m.m[14];

        *OffsetBuffer += 3;
       }

      if (ScaleBuffer != 0)
       {
        (**ScaleBuffer) = Instance->GetScale();

        (*ScaleBuffer)++;
       }
     }

    unsigned_int32                     SubBranchIndex;
    unsigned_int32                     SubBranchCount;

    SubBranchCount = BranchModel->GetSubBranchCount();

    for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
     {
      P3DHLIFillCloneTransformBufferHelper Helper(RNG,
                                            BranchModel->GetSubBranchModel(SubBranchIndex),
                                            Instance,
                                            RequiredBranch,
                                            OffsetBuffer,
                                            OrientationBuffer,
                                            ScaleBuffer);

      const_cast<P3DBranchingAlg*>(BranchModel->GetSubBranchModel(SubBranchIndex)->GetBranchingAlg())
       ->CreateBranches(&Helper,Instance,RNG);
     }

    if (StemModel != 0)
     {
      StemModel->ReleaseInstance(Instance);
     }
   }

  private          :

  P3DMathRNG                          *RNG;
  const P3DBranchModel                *BranchModel;
  const P3DStemModelInstance          *Parent;
  const P3DBranchModel                *RequiredBranch;
  float                              **OffsetBuffer;
  float                              **OrientationBuffer;
  float                              **ScaleBuffer;
 };

class P3DHLIFillVAttrBufferHelper : public P3DBranchingFactory
 {
  public           :

                   P3DHLIFillVAttrBufferHelper
                                      (P3DMathRNG         *RNG,
                                       const P3DBranchModel
                                                          *BranchModel,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       const P3DBranchModel
                                                          *RequiredBranch,
                                       unsigned_int32        Attr,
                                       unsigned char     **Buffer)
   {
    this->RNG            = RNG;
    this->BranchModel    = BranchModel;
    this->Parent         = Parent;
    this->RequiredBranch = RequiredBranch;
    this->Attr           = Attr;
    this->Buffer         = Buffer;
   }

  virtual void     GenerateBranch     (float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation)
   {
    const P3DStemModel                *StemModel;
    P3DStemModelInstance              *Instance;

    StemModel = BranchModel->GetStemModel();

    if (StemModel != 0)
     {
      Instance = StemModel->CreateInstance(RNG,Parent,Offset,Orientation);
     }
    else
     {
      Instance = 0;
     }

    if (BranchModel == RequiredBranch)
     {
      unsigned_int32                     VAttrIndex;
      unsigned_int32                     VAttrCount;

      VAttrCount = Instance->GetVAttrCount(Attr);

      for (VAttrIndex = 0; VAttrIndex < VAttrCount; VAttrIndex++)
       {
        if      (Attr == P3D_ATTR_VERTEX)
         {
          Instance->GetVAttrValue((float*)(*Buffer),P3D_ATTR_VERTEX,VAttrIndex);

          (*Buffer) += sizeof(float) * 3;
         }
        else if (Attr == P3D_ATTR_NORMAL)
         {
          Instance->GetVAttrValue((float*)(*Buffer),P3D_ATTR_NORMAL,VAttrIndex);

          (*Buffer) += sizeof(float) * 3;
         }
        else if (Attr == P3D_ATTR_TEXCOORD0)
         {
          Instance->GetVAttrValue((float*)(*Buffer),P3D_ATTR_TEXCOORD0,VAttrIndex);

          (*Buffer) += sizeof(float) * 2;
         }
        else if (Attr == P3D_ATTR_TANGENT)
         {
          Instance->GetVAttrValue((float*)(*Buffer),P3D_ATTR_TANGENT,VAttrIndex);

          (*Buffer) += sizeof(float) * 3;
         }
        else if (Attr == P3D_ATTR_BINORMAL)
         {
          Instance->GetVAttrValue((float*)(*Buffer),P3D_ATTR_BINORMAL,VAttrIndex);

          (*Buffer) += sizeof(float) * 3;
         }
       }
     }

    unsigned_int32                     SubBranchIndex;
    unsigned_int32                     SubBranchCount;

    SubBranchCount = BranchModel->GetSubBranchCount();

    for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
     {
      P3DHLIFillVAttrBufferHelper    Helper(RNG,
                                            BranchModel->GetSubBranchModel(SubBranchIndex),
                                            Instance,
                                            RequiredBranch,
                                            Attr,
                                            Buffer);

      const_cast<P3DBranchingAlg*>(BranchModel->GetSubBranchModel(SubBranchIndex)->GetBranchingAlg())
       ->CreateBranches(&Helper,Instance,RNG);
     }

    if (StemModel != 0)
     {
      StemModel->ReleaseInstance(Instance);
     }
   }

  private          :

  P3DMathRNG                          *RNG;
  const P3DBranchModel                *BranchModel;
  const P3DStemModelInstance          *Parent;
  const P3DBranchModel                *RequiredBranch;
  unsigned_int32                         Attr;
  unsigned char                      **Buffer;
 };

class P3DHLIFillVAttrBufferIHelper : public P3DBranchingFactory
 {
  public           :

                   P3DHLIFillVAttrBufferIHelper
                                      (P3DMathRNG         *RNG,
                                       const P3DBranchModel
                                                          *BranchModel,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       const P3DBranchModel
                                                          *RequiredBranch,
                                       const P3DHLIVAttrFormat
                                                          *VAttrFormat,
                                       unsigned char     **Buffer)
   {
    this->RNG            = RNG;
    this->BranchModel    = BranchModel;
    this->Parent         = Parent;
    this->RequiredBranch = RequiredBranch;
    this->VAttrFormat    = VAttrFormat;
    this->Buffer         = Buffer;
   }

  virtual void     GenerateBranch     (float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation)
   {
    const P3DStemModel                *StemModel;
    P3DStemModelInstance              *Instance;

    StemModel = BranchModel->GetStemModel();

    if (StemModel != 0)
     {
      Instance = StemModel->CreateInstance(RNG,Parent,Offset,Orientation);
     }
    else
     {
      Instance = 0;
     }

    if (BranchModel == RequiredBranch)
     {
      unsigned_int32                     VAttrIndex;
      unsigned_int32                     VAttrCount;

      VAttrCount = Instance->GetVAttrCountI();

      for (VAttrIndex = 0; VAttrIndex < VAttrCount; VAttrIndex++)
       {
        if (VAttrFormat->HasAttr(P3D_ATTR_VERTEX))
         {
          Instance->GetVAttrValueI((float*)(&((*Buffer)[VAttrFormat->GetAttrOffset(P3D_ATTR_VERTEX)])),
                                   P3D_ATTR_VERTEX,
                                   VAttrIndex);
         }

        if (VAttrFormat->HasAttr(P3D_ATTR_NORMAL))
         {
          Instance->GetVAttrValueI((float*)(&((*Buffer)[VAttrFormat->GetAttrOffset(P3D_ATTR_NORMAL)])),
                                   P3D_ATTR_NORMAL,
                                   VAttrIndex);
         }

        if (VAttrFormat->HasAttr(P3D_ATTR_TEXCOORD0))
         {
          Instance->GetVAttrValueI((float*)(&((*Buffer)[VAttrFormat->GetAttrOffset(P3D_ATTR_TEXCOORD0)])),
                                   P3D_ATTR_TEXCOORD0,
                                   VAttrIndex);
         }

        if (VAttrFormat->HasAttr(P3D_ATTR_TANGENT))
         {
          Instance->GetVAttrValueI((float*)(&((*Buffer)[VAttrFormat->GetAttrOffset(P3D_ATTR_TANGENT)])),
                                   P3D_ATTR_TANGENT,
                                   VAttrIndex);
         }

        if (VAttrFormat->HasAttr(P3D_ATTR_BINORMAL))
         {
          Instance->GetVAttrValueI((float*)(&((*Buffer)[VAttrFormat->GetAttrOffset(P3D_ATTR_BINORMAL)])),
                                   P3D_ATTR_BINORMAL,
                                   VAttrIndex);
         }

        if (VAttrFormat->HasAttr(P3D_ATTR_BILLBOARD_POS))
         {
          Instance->GetVAttrValueI((float*)(&((*Buffer)[VAttrFormat->GetAttrOffset(P3D_ATTR_BILLBOARD_POS)])),
                                   P3D_ATTR_BILLBOARD_POS,
                                   VAttrIndex);
         }

        (*Buffer) += VAttrFormat->GetStride();
       }
     }

    unsigned_int32                     SubBranchIndex;
    unsigned_int32                     SubBranchCount;

    SubBranchCount = BranchModel->GetSubBranchCount();

    for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
     {
      P3DHLIFillVAttrBufferIHelper   Helper(RNG,
                                            BranchModel->GetSubBranchModel(SubBranchIndex),
                                            Instance,
                                            RequiredBranch,
                                            VAttrFormat,
                                            Buffer);

      const_cast<P3DBranchingAlg*>(BranchModel->GetSubBranchModel(SubBranchIndex)->GetBranchingAlg())
       ->CreateBranches(&Helper,Instance,RNG);
     }

    if (StemModel != 0)
     {
      StemModel->ReleaseInstance(Instance);
     }
   }

  private          :

  P3DMathRNG                          *RNG;
  const P3DBranchModel                *BranchModel;
  const P3DStemModelInstance          *Parent;
  const P3DBranchModel                *RequiredBranch;
  const P3DHLIVAttrFormat             *VAttrFormat;
  unsigned char                      **Buffer;
 };

class P3DHLIFillVAttrBuffersIHelper : public P3DBranchingFactory
 {
  public           :

                   P3DHLIFillVAttrBuffersIHelper
                                      (P3DMathRNG         *RNG,
                                       const P3DBranchModel
                                                          *BranchModel,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       const P3DBranchModel
                                                          *RequiredBranch,
                                       const P3DHLIVAttrBuffers
                                                          *VAttrBuffers,
                                       void              **DataBuffers)
   {
    this->RNG            = RNG;
    this->BranchModel    = BranchModel;
    this->Parent         = Parent;
    this->RequiredBranch = RequiredBranch;
    this->VAttrBuffers   = VAttrBuffers;
    this->DataBuffers    = DataBuffers;
   }

  virtual void     GenerateBranch     (float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation)
   {
    const P3DStemModel                *StemModel;
    P3DStemModelInstance              *Instance;

    StemModel = BranchModel->GetStemModel();

    if (StemModel != 0)
     {
      Instance = StemModel->CreateInstance(RNG,Parent,Offset,Orientation);
     }
    else
     {
      Instance = 0;
     }

    if (BranchModel == RequiredBranch)
     {
      unsigned_int32                     VAttrIndex;
      unsigned_int32                     VAttrCount;

      VAttrCount = Instance->GetVAttrCountI();

      for (VAttrIndex = 0; VAttrIndex < VAttrCount; VAttrIndex++)
       {
        if (VAttrBuffers->HasAttr(P3D_ATTR_VERTEX))
         {
          Instance->GetVAttrValueI
           ((float*)(&(((char*)(DataBuffers[P3D_ATTR_VERTEX]))[VAttrBuffers->GetAttrOffset(P3D_ATTR_VERTEX)])),
             P3D_ATTR_VERTEX,
             VAttrIndex);

          DataBuffers[P3D_ATTR_VERTEX] = ((char*)(DataBuffers[P3D_ATTR_VERTEX])) + VAttrBuffers->GetAttrStride(P3D_ATTR_VERTEX);
         }

        if (VAttrBuffers->HasAttr(P3D_ATTR_NORMAL))
         {
          Instance->GetVAttrValueI
           ((float*)(&(((char*)(DataBuffers[P3D_ATTR_NORMAL]))[VAttrBuffers->GetAttrOffset(P3D_ATTR_NORMAL)])),
             P3D_ATTR_NORMAL,
             VAttrIndex);

          DataBuffers[P3D_ATTR_NORMAL] = ((char*)(DataBuffers[P3D_ATTR_NORMAL])) + VAttrBuffers->GetAttrStride(P3D_ATTR_NORMAL);
         }

        if (VAttrBuffers->HasAttr(P3D_ATTR_TEXCOORD0))
         {
          Instance->GetVAttrValueI
           ((float*)(&(((char*)(DataBuffers[P3D_ATTR_TEXCOORD0]))[VAttrBuffers->GetAttrOffset(P3D_ATTR_TEXCOORD0)])),
             P3D_ATTR_TEXCOORD0,
             VAttrIndex);

          DataBuffers[P3D_ATTR_TEXCOORD0] = ((char*)(DataBuffers[P3D_ATTR_TEXCOORD0])) + VAttrBuffers->GetAttrStride(P3D_ATTR_TEXCOORD0);
         }

        if (VAttrBuffers->HasAttr(P3D_ATTR_TANGENT))
         {
          Instance->GetVAttrValueI
           ((float*)(&(((char*)(DataBuffers[P3D_ATTR_TANGENT]))[VAttrBuffers->GetAttrOffset(P3D_ATTR_TANGENT)])),
             P3D_ATTR_TANGENT,
             VAttrIndex);

          DataBuffers[P3D_ATTR_TANGENT] = ((char*)(DataBuffers[P3D_ATTR_TANGENT])) + VAttrBuffers->GetAttrStride(P3D_ATTR_TANGENT);
         }

        if (VAttrBuffers->HasAttr(P3D_ATTR_BINORMAL))
         {
          Instance->GetVAttrValueI
           ((float*)(&(((char*)(DataBuffers[P3D_ATTR_BINORMAL]))[VAttrBuffers->GetAttrOffset(P3D_ATTR_BINORMAL)])),
             P3D_ATTR_BINORMAL,
             VAttrIndex);

          DataBuffers[P3D_ATTR_BINORMAL] = ((char*)(DataBuffers[P3D_ATTR_BINORMAL])) + VAttrBuffers->GetAttrStride(P3D_ATTR_BINORMAL);
         }

        if (VAttrBuffers->HasAttr(P3D_ATTR_BILLBOARD_POS))
         {
          Instance->GetVAttrValueI
           ((float*)(&(((char*)(DataBuffers[P3D_ATTR_BILLBOARD_POS]))[VAttrBuffers->GetAttrOffset(P3D_ATTR_BILLBOARD_POS)])),
             P3D_ATTR_BILLBOARD_POS,
             VAttrIndex);

          DataBuffers[P3D_ATTR_BILLBOARD_POS] = ((char*)(DataBuffers[P3D_ATTR_BILLBOARD_POS])) + VAttrBuffers->GetAttrStride(P3D_ATTR_BILLBOARD_POS);
         }
       }
     }

    unsigned_int32                     SubBranchIndex;
    unsigned_int32                     SubBranchCount;

    SubBranchCount = BranchModel->GetSubBranchCount();

    for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
     {
      P3DHLIFillVAttrBuffersIHelper  Helper(RNG,
                                            BranchModel->GetSubBranchModel(SubBranchIndex),
                                            Instance,
                                            RequiredBranch,
                                            VAttrBuffers,
                                            DataBuffers);

      const_cast<P3DBranchingAlg*>(BranchModel->GetSubBranchModel(SubBranchIndex)->GetBranchingAlg())
       ->CreateBranches(&Helper,Instance,RNG);
     }

    if (StemModel != 0)
     {
      StemModel->ReleaseInstance(Instance);
     }
   }

  private          :

  P3DMathRNG                          *RNG;
  const P3DBranchModel                *BranchModel;
  const P3DStemModelInstance          *Parent;
  const P3DBranchModel                *RequiredBranch;
  const P3DHLIVAttrBuffers            *VAttrBuffers;
  void                               **DataBuffers;
 };

class P3DHLIFillVAttrBuffersIMultiHelper : public P3DBranchingFactory
 {
  public           :

                   P3DHLIFillVAttrBuffersIMultiHelper
                                      (P3DMathRNG         *RNG,
                                       const P3DBranchModel
                                                          *BranchModel,
                                       const P3DStemModelInstance
                                                          *Parent,
                                       unsigned_int32        GroupIndex,
                                       P3DHLIVAttrBufferSet
                                                          *VAttrBufferSetArray)
   {
    this->RNG                 = RNG;
    this->BranchModel         = BranchModel;
    this->Parent              = Parent;
    this->GroupIndex          = GroupIndex;
    this->VAttrBufferSetArray = VAttrBufferSetArray;
   }

  virtual void     GenerateBranch     (float               Offset,
                                       const P3DQuaternionf
                                                          *Orientation)
   {
    const P3DStemModel                *StemModel;
    P3DStemModelInstance              *Instance;

    StemModel = BranchModel->GetStemModel();

    if (StemModel != 0)
     {
      Instance = StemModel->CreateInstance(RNG,Parent,Offset,Orientation);
     }
    else
     {
      Instance = 0;
     }

    if (Instance != 0)
     {
      unsigned_int32                     VAttrIndex;
      unsigned_int32                     VAttrCount;

      VAttrCount = Instance->GetVAttrCountI();

      for (VAttrIndex = 0; VAttrIndex < VAttrCount; VAttrIndex++)
       {
        if (VAttrBufferSetArray[GroupIndex][P3D_ATTR_VERTEX] != 0)
         {
          Instance->GetVAttrValueI
           (VAttrBufferSetArray[GroupIndex][P3D_ATTR_VERTEX],
            P3D_ATTR_VERTEX,
            VAttrIndex);

          VAttrBufferSetArray[GroupIndex][P3D_ATTR_VERTEX] += 3;
         }

        if (VAttrBufferSetArray[GroupIndex][P3D_ATTR_NORMAL] != 0)
         {
          Instance->GetVAttrValueI
           (VAttrBufferSetArray[GroupIndex][P3D_ATTR_NORMAL],
            P3D_ATTR_NORMAL,
            VAttrIndex);

          VAttrBufferSetArray[GroupIndex][P3D_ATTR_NORMAL] += 3;
         }

        if (VAttrBufferSetArray[GroupIndex][P3D_ATTR_TEXCOORD0] != 0)
         {
          Instance->GetVAttrValueI
           (VAttrBufferSetArray[GroupIndex][P3D_ATTR_TEXCOORD0],
            P3D_ATTR_TEXCOORD0,
            VAttrIndex);

          VAttrBufferSetArray[GroupIndex][P3D_ATTR_TEXCOORD0] += 2;
         }

        if (VAttrBufferSetArray[GroupIndex][P3D_ATTR_TANGENT] != 0)
         {
          Instance->GetVAttrValueI
           (VAttrBufferSetArray[GroupIndex][P3D_ATTR_TANGENT],
            P3D_ATTR_TANGENT,
            VAttrIndex);

          VAttrBufferSetArray[GroupIndex][P3D_ATTR_TANGENT] += 3;
         }

        if (VAttrBufferSetArray[GroupIndex][P3D_ATTR_BINORMAL] != 0)
         {
          Instance->GetVAttrValueI
           (VAttrBufferSetArray[GroupIndex][P3D_ATTR_BINORMAL],
            P3D_ATTR_BINORMAL,
            VAttrIndex);

          VAttrBufferSetArray[GroupIndex][P3D_ATTR_BINORMAL] += 3;
         }

        if (VAttrBufferSetArray[GroupIndex][P3D_ATTR_BILLBOARD_POS] != 0)
         {
          Instance->GetVAttrValueI
           (VAttrBufferSetArray[GroupIndex][P3D_ATTR_BILLBOARD_POS],
            P3D_ATTR_BILLBOARD_POS,
            VAttrIndex);

          VAttrBufferSetArray[GroupIndex][P3D_ATTR_BILLBOARD_POS] += 3;
         }
       }
     }

    unsigned_int32                     SubBranchIndex;
    unsigned_int32                     SubBranchCount;
    unsigned_int32                     SubGroupIndex;

    if (StemModel != 0)
     {
      SubGroupIndex = GroupIndex + 1;
     }
    else
     {
      SubGroupIndex = 0;
     }

    SubBranchCount = BranchModel->GetSubBranchCount();

    for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
     {
      P3DHLIFillVAttrBuffersIMultiHelper
                                       Helper(RNG,
                                              BranchModel->GetSubBranchModel(SubBranchIndex),
                                              Instance,
                                              SubGroupIndex,
                                              VAttrBufferSetArray);

      const_cast<P3DBranchingAlg*>(BranchModel->GetSubBranchModel(SubBranchIndex)->GetBranchingAlg())
       ->CreateBranches(&Helper,Instance,RNG);

      SubGroupIndex += CalcInternalGroupCount
                        (BranchModel->GetSubBranchModel(SubBranchIndex));
     }

    if (Instance != 0)
     {
      StemModel->ReleaseInstance(Instance);
     }
   }

  private          :

  P3DMathRNG                          *RNG;
  const P3DBranchModel                *BranchModel;
  const P3DStemModelInstance          *Parent;
  unsigned_int32                         GroupIndex;
  P3DHLIVAttrBufferSet                *VAttrBufferSetArray;
 };

                   P3DHLIVAttrFormat::P3DHLIVAttrFormat
                                      (unsigned_int32        Stride)
 {
  for (unsigned_int32 Index = 0; Index < P3D_MAX_ATTRS; Index++)
   {
    Enabled[Index] = false;
    Offsets[Index] = 0;
   }

  this->Stride = Stride;
 }

bool               P3DHLIVAttrFormat::HasAttr
                                      (unsigned_int32        Attr) const
 {
  if (Attr < P3D_MAX_ATTRS)
   {
    return(Enabled[Attr]);
   }
  else
   {
    throw P3DExceptionGeneric("invalid vertex attribute");
   }
 }

unsigned_int32       P3DHLIVAttrFormat::GetAttrOffset
                                      (unsigned_int32        Attr) const
 {
  if (Attr < P3D_MAX_ATTRS)
   {
    return(Offsets[Attr]);
   }
  else
   {
    throw P3DExceptionGeneric("invalid vertex attribute");
   }
 }

unsigned_int32       P3DHLIVAttrFormat::GetStride
                                      () const
 {
  return(Stride);
 }

void               P3DHLIVAttrFormat::AddAttr
                                      (unsigned_int32        Attr,
                                       unsigned_int32        Offset)
 {
  if (Attr < P3D_MAX_ATTRS)
   {
    Enabled[Attr] = true;
    Offsets[Attr] = Offset;
   }
  else
   {
    throw P3DExceptionGeneric("invalid vertex attribute");
   }
 }

static void        CheckVAttrValidity (unsigned_int32        Attr)
 {
  if (Attr >= P3D_MAX_ATTRS)
   {
    throw P3DExceptionGeneric("invalid vertex attribute");
   }
 }

                   P3DHLIVAttrBuffers::P3DHLIVAttrBuffers
                                      ()
 {
  for (unsigned_int32 Index = 0; Index < P3D_MAX_ATTRS; Index++)
   {
    Buffers[Index] = 0;
    Offsets[Index] = 0;
    Strides[Index] = 0;
   }
 }

void               P3DHLIVAttrBuffers::AddAttr
                                      (unsigned_int32        Attr,
                                       void               *Data,
                                       unsigned_int32        Offset,
                                       unsigned_int32        Stride)
 {
  CheckVAttrValidity(Attr);

  Buffers[Attr] = Data;
  Offsets[Attr] = Offset;
  Strides[Attr] = Stride;
 }

bool               P3DHLIVAttrBuffers::HasAttr
                                      (unsigned_int32        Attr) const
 {
  CheckVAttrValidity(Attr);

  return(Buffers[Attr] != 0);
 }

void              *P3DHLIVAttrBuffers::GetAttrBuffer
                                      (unsigned_int32        Attr) const
 {
  CheckVAttrValidity(Attr);

  return(Buffers[Attr]);
 }

unsigned_int32       P3DHLIVAttrBuffers::GetAttrOffset
                                      (unsigned_int32        Attr) const
 {
  CheckVAttrValidity(Attr);

  return(Offsets[Attr]);
 }

unsigned_int32       P3DHLIVAttrBuffers::GetAttrStride
                                      (unsigned_int32        Attr) const
 {
  CheckVAttrValidity(Attr);

  return(Strides[Attr]);
 }

static
const P3DBranchModel
                  *GetBranchModelByIndex
                                      (const P3DPlantModel*Model,
                                       unsigned_int32        Index)
 {
  const P3DBranchModel                *BranchModel;

  BranchModel = P3DPlantModel::GetBranchModelByIndex(Model,Index);

  if (BranchModel == 0)
   {
    throw P3DExceptionGeneric("group index out of range");
   }

  return(BranchModel);
 }

                   P3DHLIPlantTemplate::P3DHLIPlantTemplate
                                      (P3DInputStringStream
                                                          *SourceStream)
 {
  P3DHLIMatFactory                     MaterialFactory;

  OwnedModel.Load(SourceStream,&MaterialFactory);
  Model = &OwnedModel;
 }

                   P3DHLIPlantTemplate::P3DHLIPlantTemplate
                                      (const P3DPlantModel*SourceModel)
 {
  Model = SourceModel;
 }

unsigned_int32       P3DHLIPlantTemplate::GetGroupCount
                                      () const
 {
  return(CalcInternalGroupCount(Model->GetPlantBase()) - 1);
 }

const char        *P3DHLIPlantTemplate::GetGroupName
                                      (unsigned_int32        GroupIndex) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->GetName());
 }

const
P3DMaterialDef    *P3DHLIPlantTemplate::GetMaterial
                                      (unsigned_int32        GroupIndex) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->
          GetMaterialInstance()->GetMaterialDef());
 }

void               P3DHLIPlantTemplate::GetBillboardSize
                                      (float              *Width,
                                       float              *Height,
                                       unsigned_int32        GroupIndex) const
 {
  const P3DBranchModel                *BranchModel;

  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  const P3DStemModelQuad *QuadModel = dynamic_cast<const P3DStemModelQuad*>(BranchModel->GetStemModel());

  if (QuadModel != 0)
   {
    if (QuadModel->IsBillboard())
     {
      *Width  = QuadModel->GetWidth();
      *Height = QuadModel->GetLength();

      return;
     }
   }

  throw P3DExceptionGeneric("trying to get billboard size for non-billboard branches");
 }

bool               P3DHLIPlantTemplate::IsCloneable
                                      (unsigned_int32        GroupIndex,
                                       bool                AllowScaling) const
 {
  return GetBranchModelByIndex(Model,GroupIndex)->GetStemModel()->IsCloneable(AllowScaling);
 }

bool               P3DHLIPlantTemplate::IsLODVisRangeEnabled
                                      (unsigned_int32        GroupIndex) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->
          GetVisRangeState()->IsEnabled());
 }

void               P3DHLIPlantTemplate::GetLODVisRange
                                      (float              *MinLOD,
                                       float              *MaxLOD,
                                       unsigned_int32        GroupIndex) const
 {
  GetBranchModelByIndex(Model,GroupIndex)->
   GetVisRangeState()->GetRange(MinLOD,MaxLOD);
 }

unsigned_int32       P3DHLIPlantTemplate::GetVAttrCount
                                      (unsigned_int32        GroupIndex,
                                       unsigned_int32        Attr) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->
          GetStemModel()->GetVAttrCount(Attr));
 }

void               P3DHLIPlantTemplate::FillCloneVAttrBuffer
                                      (void               *VAttrBuffer,
                                       unsigned_int32        GroupIndex,
                                       unsigned_int32        Attr) const
 {
  GetBranchModelByIndex(Model,GroupIndex)->GetStemModel()->
   FillCloneVAttrBuffer(VAttrBuffer,Attr);
 }

unsigned_int32       P3DHLIPlantTemplate::GetPrimitiveCount
                                      (unsigned_int32        GroupIndex) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->
          GetStemModel()->GetPrimitiveCount());
 }

unsigned_int32       P3DHLIPlantTemplate::GetPrimitiveType
                                      (unsigned_int32        GroupIndex,
                                       unsigned_int32        PrimitiveIndex) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->
          GetStemModel()->GetPrimitiveType(PrimitiveIndex));
 }

void               P3DHLIPlantTemplate::FillVAttrIndexBuffer
                                      (void               *IndexBuffer,
                                       unsigned_int32        GroupIndex,
                                       unsigned_int32        Attr,
                                       unsigned_int32        ElementType,
                                       unsigned_int32        IndexBase) const
 {
  GetBranchModelByIndex(Model,GroupIndex)->GetStemModel()->
   FillVAttrIndexBuffer(IndexBuffer,Attr,ElementType,IndexBase);
 }

unsigned_int32       P3DHLIPlantTemplate::GetVAttrCountI
                                      (unsigned_int32        GroupIndex) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->
          GetStemModel()->GetVAttrCountI());
 }

void               P3DHLIPlantTemplate::FillCloneVAttrBuffersI
                                      (const P3DHLIVAttrBuffers
                                                          *VAttrBuffers,
                                       unsigned_int32        GroupIndex) const
 {
  const P3DStemModel                  *StemModel;
  unsigned_int32                         AttrIndex;

  StemModel = GetBranchModelByIndex(Model,GroupIndex)->GetStemModel();

  for (AttrIndex = 0; AttrIndex < P3D_MAX_ATTRS; AttrIndex++)
   {
    if (VAttrBuffers->HasAttr(AttrIndex))
     {
      StemModel->FillCloneVAttrBufferI
       (&((char*)(VAttrBuffers->GetAttrBuffer(AttrIndex)))[VAttrBuffers->GetAttrOffset(AttrIndex)],
        AttrIndex,
        VAttrBuffers->GetAttrStride(AttrIndex));
     }
   }
 }

unsigned_int32       P3DHLIPlantTemplate::GetIndexCount
                                      (unsigned_int32        GroupIndex,
                                       unsigned_int32        PrimitiveType) const
 {
  return(GetBranchModelByIndex(Model,GroupIndex)->
          GetStemModel()->GetIndexCount(PrimitiveType));
 }

void               P3DHLIPlantTemplate::FillIndexBuffer
                                      (void               *IndexBuffer,
                                       unsigned_int32        GroupIndex,
                                       unsigned_int32        PrimitiveType,
                                       unsigned_int32        ElementType,
                                       unsigned_int32        IndexBase) const
 {
  GetBranchModelByIndex(Model,GroupIndex)->GetStemModel()->
   FillIndexBuffer(IndexBuffer,PrimitiveType,ElementType,IndexBase);
 }

P3DHLIPlantInstance
                  *P3DHLIPlantTemplate::CreateInstance
                                      (unsigned_int32        BaseSeed) const
 {
  if (BaseSeed == 0)
   {
    return(new P3DHLIPlantInstance(Model,Model->GetBaseSeed()));
   }
  else
   {
    return(new P3DHLIPlantInstance(Model,BaseSeed));
   }
 }

                   P3DHLIPlantInstance::P3DHLIPlantInstance
                                      (const P3DPlantModel*Model,
                                       unsigned_int32        BaseSeed)
 {
  this->Model    = Model;
  this->BaseSeed = BaseSeed;
 }

unsigned_int32       P3DHLIPlantInstance::GetBranchCount
                                      (unsigned_int32        GroupIndex) const
 {
  const P3DBranchModel                *BranchModel;
  unsigned_int32                         Counter;

  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  Counter = 0;

  P3DMathRNGSimple                     RNG(BaseSeed);

  P3DHLIBranchCalculator               Calculator( IsRandomnessEnabled() ? &RNG : 0,
                                                   Model->GetPlantBase(),
                                                   0,
                                                   BranchModel,
                                                  &Counter);

  Calculator.GenerateBranch(0.0f,0);

  return(Counter);
 }

void               P3DHLIPlantInstance::GetBranchCountMulti
                                      (unsigned_int32       *BranchCounts) const
 {
  unsigned_int32                         GroupIndex;
  unsigned_int32                         GroupCount;

  GroupCount = CalcInternalGroupCount(Model->GetPlantBase()) - 1;

  for (GroupIndex = 0; GroupIndex < GroupCount; GroupIndex++)
   {
    BranchCounts[GroupIndex] = 0;
   }

  P3DMathRNGSimple                     RNG(Model->GetBaseSeed());

  P3DHLIBranchCalculatorMulti Calculator(IsRandomnessEnabled() ? &RNG : 0,
                                          Model->GetPlantBase(),
                                          0,
                                          0,
                                          BranchCounts);

  Calculator.GenerateBranch(0.0f,0);
 }

class P3DBranchingFactoryBoundCalc : public P3DBranchingFactory
 {
  public           :

                   P3DBranchingFactoryBoundCalc    (P3DBranchModel  *BranchModel,
                                                    P3DStemModelInstance
                                                                    *ParentStem,
                                                    P3DMathRNG      *RNG,
                                                    float           *Min,
                                                    float           *Max);

  virtual void     GenerateBranch     (float               offset,
                                       const P3DQuaternionf
                                                          *orientation);

  private          :

  P3DBranchModel                      *BranchModel;
  P3DStemModelInstance                *ParentStem;
  P3DMathRNG                          *RNG;
  float                               *Min;
  float                               *Max;
 };

                   P3DBranchingFactoryBoundCalc::P3DBranchingFactoryBoundCalc
                                                   (P3DBranchModel  *BranchModel,
                                                    P3DStemModelInstance
                                                                    *ParentStem,
                                                    P3DMathRNG      *RNG,
                                                    float           *Min,
                                                    float           *Max)
 {
  this->BranchModel = BranchModel;
  this->ParentStem  = ParentStem;
  this->RNG         = RNG;
  this->Min         = Min;
  this->Max         = Max;
 }

void               P3DBranchingFactoryBoundCalc::GenerateBranch
                                      (float               offset,
                                       const P3DQuaternionf
                                                          *orientation)
 {
  unsigned_int32                         SubBranchIndex;
  unsigned_int32                         SubBranchCount;
  P3DBranchModel                      *SubBranchModel;
  P3DBranchingAlg                     *BranchingAlg;
  P3DStemModel                        *StemModel;
  P3DStemModelInstance                *StemInstance;

  StemModel = BranchModel->GetStemModel();

  if (StemModel != 0)
   {
    float          InstMin[3];
    float          InstMax[3];

    StemInstance = StemModel->CreateInstance
                    (RNG,ParentStem,offset,orientation);

    StemInstance->GetBoundBox(InstMin,InstMax);

    for (unsigned_int32 Axis = 0; Axis < 3; Axis++)
     {
      if      (InstMin[Axis] < Min[Axis])
       {
        Min[Axis] = InstMin[Axis];
       }

      if (InstMax[Axis] > Max[Axis])
       {
        Max[Axis] = InstMax[Axis];
       }
     }
   }
  else
   {
    StemInstance = 0;
   }

  SubBranchCount = BranchModel->GetSubBranchCount();

  for (SubBranchIndex = 0; SubBranchIndex < SubBranchCount; SubBranchIndex++)
   {
    SubBranchModel = BranchModel->GetSubBranchModel(SubBranchIndex);
    BranchingAlg   = SubBranchModel->GetBranchingAlg();

    P3DBranchingFactoryBoundCalc         BranchingFactory(SubBranchModel,StemInstance,RNG,Min,Max);

    BranchingAlg->CreateBranches(&BranchingFactory,StemInstance,RNG);
   }

  if (StemModel != 0)
   {
    StemModel->ReleaseInstance(StemInstance);
   }
 }

static void        P3DHLICalcBBox     (float              *Min,
                                       float              *Max,
                                       const P3DPlantModel*Model,
                                       unsigned_int32        BaseSeed)
 {
  P3DMathRNGSimple                     RNG(BaseSeed);

  Min[0] = Min[1] = Min[2] = 0.0f;
  Max[0] = Max[1] = Max[2] = 0.0f;

  P3DBranchingFactoryBoundCalc
   BranchingFactory ((const_cast<P3DPlantModel*>(Model))->GetPlantBase(),
                     0,
                     (Model->GetFlags() & P3D_MODEL_FLAG_NO_RANDOMNESS) ? 0 : &RNG,
                     Min,
                     Max);

  BranchingFactory.GenerateBranch(0.0f,0);
 }

void               P3DHLIPlantInstance::GetBoundingBox
                                      (float              *Min,
                                       float              *Max) const
 {
  P3DHLICalcBBox(Min,Max,Model,BaseSeed);
 }

void               P3DHLIPlantInstance::FillCloneTransformBuffer
                                      (float              *OffsetBuffer,
                                       float              *OrientationBuffer,
                                       float              *ScaleBuffer,
                                       unsigned_int32        GroupIndex) const
 {
  const P3DBranchModel                *BranchModel;

  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  P3DMathRNGSimple RNG(BaseSeed);

  P3DHLIFillCloneTransformBufferHelper Helper(IsRandomnessEnabled() ? &RNG : 0,
                                              Model->GetPlantBase(),
                                              0,
                                              BranchModel,
                                              OffsetBuffer != 0 ? &OffsetBuffer : 0,
                                              OrientationBuffer != 0 ? &OrientationBuffer : 0,
                                              ScaleBuffer != 0 ? &ScaleBuffer : 0);

  Helper.GenerateBranch(0.0f,0);
 }

unsigned_int32       P3DHLIPlantInstance::GetVAttrCount
                                      (unsigned_int32        GroupIndex,
                                       unsigned_int32        Attr) const
 {
  unsigned_int32                         BranchCount;
  const P3DBranchModel                *BranchModel;

  BranchCount = GetBranchCount(GroupIndex);
  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  return(BranchCount * BranchModel->GetStemModel()->GetVAttrCount(Attr));
 }

void               P3DHLIPlantInstance::FillVAttrBuffer
                                      (void               *VAttrBuffer,
                                       unsigned_int32        GroupIndex,
                                       unsigned_int32        Attr) const
 {
  const P3DBranchModel                *BranchModel;

  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  P3DMathRNGSimple                     RNG(BaseSeed);
  unsigned char                       *Buffer;

  Buffer = (unsigned char*)VAttrBuffer;

  P3DHLIFillVAttrBufferHelper Helper( IsRandomnessEnabled() ? &RNG : 0,
                                      Model->GetPlantBase(),
                                      0,
                                      BranchModel,
                                      Attr,
                                     &Buffer);

  Helper.GenerateBranch(0.0f,0);
 }

unsigned_int32       P3DHLIPlantInstance::GetVAttrCountI
                                      (unsigned_int32        GroupIndex) const
 {
  unsigned_int32                         BranchCount;
  const P3DBranchModel                *BranchModel;

  BranchCount = GetBranchCount(GroupIndex);
  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  return(BranchCount * BranchModel->GetStemModel()->GetVAttrCountI());
 }

void               P3DHLIPlantInstance::FillVAttrBufferI
                                      (void               *VAttrBuffer,
                                       unsigned_int32        GroupIndex,
                                       const P3DHLIVAttrFormat
                                                          *VAttrFormat) const
 {
  const P3DBranchModel                *BranchModel;

  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  P3DMathRNGSimple                     RNG(BaseSeed);
  unsigned char                       *Buffer;

  Buffer = (unsigned char*)VAttrBuffer;

  P3DHLIFillVAttrBufferIHelper Helper( IsRandomnessEnabled() ? &RNG : 0,
                                       Model->GetPlantBase(),
                                       0,
                                       BranchModel,
                                       VAttrFormat,
                                      &Buffer);

  Helper.GenerateBranch(0.0f,0);
 }

void               P3DHLIPlantInstance::FillVAttrBuffersI
                                      (const P3DHLIVAttrBuffers
                                                          *VAttrBuffers,
                                       unsigned_int32        GroupIndex) const
 {
  const P3DBranchModel                *BranchModel;

  BranchModel = GetBranchModelByIndex(Model,GroupIndex);

  P3DMathRNGSimple                     RNG(BaseSeed);
  void                                *DataBuffers[P3D_MAX_ATTRS];

  for (unsigned_int32 AttrIndex = 0; AttrIndex < P3D_MAX_ATTRS; AttrIndex++)
   {
    DataBuffers[AttrIndex] = VAttrBuffers->GetAttrBuffer(AttrIndex);
   }

  P3DHLIFillVAttrBuffersIHelper Helper(IsRandomnessEnabled() ? &RNG : 0,
                                       Model->GetPlantBase(),
                                       0,
                                       BranchModel,
                                       VAttrBuffers,
                                       DataBuffers);

  Helper.GenerateBranch(0.0f,0);
 }

void               P3DHLIPlantInstance::FillVAttrBuffersIMulti
                                      (P3DHLIVAttrBufferSet
                                                          *VAttrBufferSet) const
 {
  unsigned_int32                         GroupIndex;
  unsigned_int32                         GroupCount;

  GroupCount = CalcInternalGroupCount(Model->GetPlantBase()) - 1;

  if (GroupCount > 0)
   {
    P3DHLIVAttrBufferSet              *TempVAttrBufferSet;

    TempVAttrBufferSet = new P3DHLIVAttrBufferSet[GroupCount];

    for (GroupIndex = 0; GroupIndex < GroupCount; GroupIndex++)
     {
      for (unsigned_int32 AttrIndex = 0; AttrIndex < P3D_MAX_ATTRS; AttrIndex++)
       {
        TempVAttrBufferSet[GroupIndex][AttrIndex] =
         VAttrBufferSet[GroupIndex][AttrIndex];
       }
     }

    P3DMathRNGSimple                     RNG(Model->GetBaseSeed());
    P3DHLIFillVAttrBuffersIMultiHelper   Helper(IsRandomnessEnabled() ? &RNG : 0,
                                                 Model->GetPlantBase(),
                                                 0,
                                                 0,
                                                 TempVAttrBufferSet);

    Helper.GenerateBranch(0.0f,0);

    delete TempVAttrBufferSet;
   }
 }

bool               P3DHLIPlantInstance::IsRandomnessEnabled() const
 {
  return (Model->GetFlags() & P3D_MODEL_FLAG_NO_RANDOMNESS) == 0;
 }

