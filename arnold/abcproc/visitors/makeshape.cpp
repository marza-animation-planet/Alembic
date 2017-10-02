#include <visitors/makeshape.h>
#include <nurbs.h>

MakeShape::MakeShape(Dso *dso)
   : mDso(dso)
   , mNode(0)
{
}

AtNode* MakeShape::createArnoldNode(const AtString &nodeType, AlembicNode &node, bool useProcName) const
{
   AbcProcGlobalLock::Acquire();

   std::string name;

   if (useProcName)
   {
      name = AiNodeGetName(mDso->procNode());
      // Rename source procedural node
      AiNodeSetStr(mDso->procNode(), Strings::name, mDso->uniqueName("_"+name).c_str());
   }
   else
   {
      name = mDso->uniqueName(node.formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|'));
   }

   AtNode *anode = AiNode(nodeType, name.c_str(), mDso->procNode());

   AbcProcGlobalLock::Release();

   return anode;
}

void MakeShape::collectUserAttributes(Alembic::Abc::ICompoundProperty userProps,
                                      Alembic::Abc::ICompoundProperty geomParams,
                                      double t,
                                      bool interpolate,
                                      UserAttributes *objectAttrs,
                                      UserAttributes *primitiveAttrs,
                                      UserAttributes *pointAttrs,
                                      UserAttributes *vertexAttrs,
                                      std::map<std::string, Alembic::AbcGeom::IV2fGeomParam> *UVs,
                                      MakeShape::CollectFilter filter,
                                      void *filterArgs)
{
   if (userProps.valid() && objectAttrs)
   {
      for (size_t i=0; i<userProps.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = userProps.getPropertyHeader(i);
         
         if (filter && !filter(header, filterArgs))
         {
            continue;
         }

         std::pair<std::string, UserAttribute> ua;

         ua.first = header.getName();
         mDso->cleanAttribName(ua.first);

         if (mDso->ignoreAttribute(ua.first))
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Ignore \"%s\" object attribute.", header.getName().c_str());
            }
            continue;
         }

         InitUserAttribute(ua.second);

         //ua.second.arnoldCategory = AI_USERDEF_CONSTANT;

         if (ReadUserAttribute(ua.second, userProps, header, t, false, interpolate))
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read \"%s\" object attribute as \"%s\"",
                         header.getName().c_str(), ua.first.c_str());
            }
            objectAttrs->insert(ua);
         }
         else
         {
            DestroyUserAttribute(ua.second);
         }
      }
   }
   
   if (geomParams.valid())
   {
      std::set<std::string> ignorePointNames;
      std::set<std::string> ignoreVertexNames;
      
      if (!mDso->outputReference() || (mDso->referenceSource() != RS_attributes &&
                                       mDso->referenceSource() != RS_attributes_then_file))
      {
         // force ignoring reference attributes, those will be set from a difference frame or file
         ignorePointNames.insert(mDso->referencePositionName());
         ignorePointNames.insert(mDso->referenceNormalName());
         ignoreVertexNames.insert(mDso->referenceNormalName());
      }
      
      // what about 'radius' and 'size'?
      
      for (size_t i=0; i<geomParams.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = geomParams.getPropertyHeader(i);
         
         Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
         
         if (UVs &&
             scope == Alembic::AbcGeom::kFacevaryingScope &&
             Alembic::AbcGeom::IV2fGeomParam::matches(header) &&
             header.getMetaData().get("notUV") != "1")
             //Alembic::AbcGeom::isUV(header))
         {
            if (filter && !filter(header, filterArgs))
            {
               continue;
            }
            
            std::pair<std::string, Alembic::AbcGeom::IV2fGeomParam> UV;
            
            UV.first = header.getName();
            mDso->cleanAttribName(UV.first);

            if (mDso->ignoreAttribute(UV.first))
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Ignore UVs \"%s\".", header.getName().c_str());
               }
               continue;
            }

            UV.second = Alembic::AbcGeom::IV2fGeomParam(geomParams, header.getName());
            UVs->insert(UV);
         }
         else
         {
            std::pair<std::string, UserAttribute> ua;

            UserAttributes *attrs = 0;

            ua.first = header.getName();
            mDso->cleanAttribName(ua.first);

            if (mDso->ignoreAttribute(ua.first))
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Ignore attribute \"%s\".", header.getName().c_str());
               }
               continue;
            }

            InitUserAttribute(ua.second);
            
            bool forceConstant = mDso->forceConstantAttribute(ua.first);
            
            switch (scope)
            {
            case Alembic::AbcGeom::kFacevaryingScope:
               if (forceConstant)
               {
                  if (objectAttrs)
                  {
                     attrs = objectAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" vertex attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               else
               {
                  if (vertexAttrs && ignoreVertexNames.find(ua.first) == ignoreVertexNames.end())
                  {
                     //ua.second.arnoldCategory = AI_USERDEF_INDEXED;
                     attrs = vertexAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" vertex attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               break;
            case Alembic::AbcGeom::kVaryingScope:
            case Alembic::AbcGeom::kVertexScope:
               if (forceConstant)
               {
                  if (objectAttrs)
                  {
                     attrs = objectAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" point attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               else
               {
                  if (pointAttrs && ignorePointNames.find(ua.first) == ignorePointNames.end())
                  {
                     //ua.second.arnoldCategory = AI_USERDEF_VARYING;
                     attrs = pointAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" point attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               break;
            case Alembic::AbcGeom::kUniformScope:
               if (forceConstant)
               {
                  if (objectAttrs)
                  {
                     attrs = objectAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" primitive attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               else
               {
                  if (primitiveAttrs)
                  {
                     //ua.second.arnoldCategory = AI_USERDEF_UNIFORM;
                     attrs = primitiveAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" primitive attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               break;
            case Alembic::AbcGeom::kConstantScope:
               if (objectAttrs)
               {
                  //ua.second.arnoldCategory = AI_USERDEF_CONSTANT;
                  attrs = objectAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" object attribute as \"%s\"",
                               header.getName().c_str(), ua.first.c_str());
                  }
               }
               break;
            default:
               continue;
            }
            
            if (attrs)
            {
               if (filter && !filter(header, filterArgs))
               {
                  DestroyUserAttribute(ua.second);
               }
               else
               {
                  if (ReadUserAttribute(ua.second, geomParams, header, t, true, interpolate))
                  {
                     if (!forceConstant)
                     {
                        attrs->insert(ua);
                     }
                     else
                     {
                        std::pair<std::string, UserAttribute> pua;
                        
                        bool promoted =  PromoteToObjectAttrib(ua.second, pua.second);
                        
                        DestroyUserAttribute(ua.second);
                        
                        if (!promoted)
                        {
                           if (mDso->verbose())
                           {
                              AiMsgWarning("[abcproc] Failed (could not promote to constant)");
                           }
                        }
                        else
                        {
                           pua.first = ua.first;
                           
                           if (mDso->verbose())
                           {
                              AiMsgInfo("[abcproc] \"%s\" promoted to constant attribute", pua.first.c_str());
                           }
                           
                           attrs->insert(pua);
                        }
                     }
                  }
                  else
                  {
                     if (mDso->verbose())
                     {
                        AiMsgWarning("[abcproc] Failed");
                     }
                     DestroyUserAttribute(ua.second);
                  }
               }
            }
         }
      }
   }
}

void MakeShape::removeConflictingAttribs(AtNode *atnode, UserAttributes *obja, UserAttributes *prma, UserAttributes *pnta, UserAttributes *vtxa, bool verbose)
{
   const AtNodeEntry *entry = AiNodeGetNodeEntry(atnode);
   
   if (!entry)
   {
      return;
   }
   
   UserAttributes::iterator attrit;
   
   AtParamIterator *paramit = AiNodeEntryGetParamIterator(entry);
   
   while (!AiParamIteratorFinished(paramit))
   {
      const AtParamEntry *pentry = AiParamIteratorGetNext(paramit);
      
      const char *tmp = AiParamGetName(pentry);
      
      if (!tmp)
      {
         continue;
      }
      
      std::string pname = tmp;
      
      if (obja)
      {
         attrit = obja->find(pname);
         if (attrit != obja->end())
         {
            if (verbose)
            {
               AiMsgInfo("[abcproc] Ignore conflicting object attribute \"%s\".", tmp);
            }
            DestroyUserAttribute(attrit->second);
            obja->erase(attrit);
         }
      }
      
      if (prma)
      {
         attrit = prma->find(pname);
         if (attrit != prma->end())
         {
            if (verbose)
            {
               AiMsgInfo("[abcproc] Ignore conflicting primitive attribute \"%s\".", tmp);
            }
            DestroyUserAttribute(attrit->second);
            prma->erase(attrit);
         }
      }
      
      if (pnta)
      {
         attrit = pnta->find(pname);
         if (attrit != pnta->end())
         {
            if (verbose)
            {
               AiMsgInfo("[abcproc] Ignore conflicting point attribute \"%s\".", tmp);
            }
            DestroyUserAttribute(attrit->second);
            pnta->erase(attrit);
         }
      }
      
      if (vtxa)
      {
         attrit = vtxa->find(pname);
         if (attrit != vtxa->end())
         {
            if (verbose)
            {
               AiMsgInfo("[abcproc] Ignore conflicting vertex attribute \"%s\".", tmp);
            }
            DestroyUserAttribute(attrit->second);
            vtxa->erase(attrit);
         }
      }
   }
   
   AiParamIteratorDestroy(paramit);
}

float* MakeShape::computeMeshSmoothNormals(MakeShape::MeshInfo &info,
                                           const float *P0,
                                           const float *P1,
                                           float blend)
{
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Compute smooth normals");
   }
   
   size_t bytesize = 3 * info.pointCount * sizeof(float);
   
   float *smoothNormals = (float*) AiMalloc(bytesize);
   
   memset(smoothNormals, 0, bytesize);

   Alembic::Abc::V3f fN, c, p0, p1, p2, e0, e1;
   
   for (unsigned int f=0, v=0; f<info.polygonCount; ++f)
   {
      unsigned int nfv = info.polygonVertexCount[f];

      if (nfv == 0)
      {
         continue;
      }

      // Compute mass center
      c.setValue(0.0f, 0.0f, 0.0f);
      for (unsigned int fv=0; fv<nfv; ++fv)
      {
         const float *P = P0 + 3 * info.vertexPointIndex[v + fv];
         c.x += P[0];
         c.y += P[1];
         c.z += P[2];
      }
      c /= float(nfv);

      // Compute face normal
      fN.setValue(0.0f, 0.0f, 0.0f);
      for (unsigned int fv=0; fv<nfv; ++fv)
      {
         unsigned int pi0 = 3 * info.vertexPointIndex[v + fv];
         unsigned int pi1 = 3 * info.vertexPointIndex[v + ((fv + 1) >= nfv ? 0 : (fv + 1))];
         const float *P0a = P0 + pi0;
         const float *P0b = P0 + pi1;

         if (P1)
         {
            float iblend = (1.0f - blend);
            const float *P1a = P1 + pi0;
            const float *P1b = P1 + pi1;

            p0.x = iblend * P0a[0] + blend * P1a[0];
            p0.y = iblend * P0a[1] + blend * P1a[1];
            p0.z = iblend * P0a[2] + blend * P1a[2];
            p1.x = iblend * P0b[0] + blend * P1b[0];
            p1.y = iblend * P0b[1] + blend * P1b[1];
            p1.z = iblend * P0b[2] + blend * P1b[2];
         }
         else
         {
            p0.x = P0a[0];
            p0.y = P0a[1];
            p0.z = P0a[2];
            p1.x = P0b[0];
            p1.y = P0b[1];
            p1.z = P0b[2];
         }

         e0 = p0 - c;
         e1 = p1 - c;

         fN += (mDso->reverseWinding() ? e0.cross(e1) : e1.cross(e0));
      }

      // Accumulate smooth normal (per point)
      // fN.length() = 2.0 * area
      // don't normalize so that the final normal is area weighted

      for (unsigned int fv=0; fv<nfv; ++fv)
      {
         unsigned int pi = 3 * info.vertexPointIndex[v + fv];
         smoothNormals[pi + 0] += fN.x;
         smoothNormals[pi + 1] += fN.y;
         smoothNormals[pi + 2] += fN.z;
      }
      
      v += nfv;
   }
   
   for (unsigned int p=0, off=0; p<info.pointCount; ++p, off+=3)
   {
      float l = smoothNormals[off  ] * smoothNormals[off  ] +
                smoothNormals[off+1] * smoothNormals[off+1] +
                smoothNormals[off+2] * smoothNormals[off+2];
      
      if (l > 0.0f)
      {
         l = 1.0f / sqrtf(l);
         
         smoothNormals[off  ] *= l;
         smoothNormals[off+1] *= l;
         smoothNormals[off+2] *= l;
      }
   }
   
   return smoothNormals;
}

bool MakeShape::FilterReferenceAttributes(const Alembic::AbcCoreAbstract::PropertyHeader &header, void *args)
{
   Dso *dso = (Dso*) args;
   
   if (!dso)
   {
      return false;
   }
   else if (header.getName() == dso->referencePositionName())
   {
      Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
      
      return (scope == Alembic::AbcGeom::kVaryingScope || scope == Alembic::AbcGeom::kVertexScope);
   }
   else if (header.getName() == dso->referenceNormalName())
   {
      Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
      
      return (scope != Alembic::AbcGeom::kConstantScope && scope != Alembic::AbcGeom::kUniformScope);
   }
   else
   {
      return false;
   }
}

bool MakeShape::checkReferenceNormals(MeshInfo &info)
{
   bool hasNref = false;
   const std::string &NrefName = mDso->referenceNormalName();
   UserAttributes::iterator uait = info.vertexAttrs.find(NrefName);
   
   if (uait != info.vertexAttrs.end())
   {
      if (isIndexedFloat3(info, uait->second))
      {
         hasNref = true;
         
         // destroy point class data if any
         uait = info.pointAttrs.find(NrefName);
         if (uait != info.pointAttrs.end())
         {
            DestroyUserAttribute(uait->second);
            info.pointAttrs.erase(uait);
         }
      }
      else
      {
         DestroyUserAttribute(uait->second);
         info.vertexAttrs.erase(uait);
      }
   }
   
   if (!hasNref)
   {
      uait = info.pointAttrs.find(NrefName);
      
      if (uait != info.pointAttrs.end())
      {
         if (isVaryingFloat3(info, uait->second))
         {
            hasNref = true;
         }
         else
         {
            DestroyUserAttribute(uait->second);
            info.pointAttrs.erase(uait);
         }
      }
   }
   
   return hasNref;
}

bool MakeShape::readReferenceNormals(AlembicMesh *refMesh,
                                     MeshInfo &info,
                                     const Alembic::Abc::M44d &Mref)
{
   Alembic::AbcGeom::IPolyMeshSchema meshSchema = refMesh->typedObject().getSchema();
   
   Alembic::AbcGeom::IN3fGeomParam Nref = meshSchema.getNormalsParam();
   
   const std::string &NrefName = mDso->referenceNormalName();
   
   if (Nref.valid() && (Nref.getScope() == Alembic::AbcGeom::kFacevaryingScope ||
                        Nref.getScope() == Alembic::AbcGeom::kVertexScope ||
                        Nref.getScope() == Alembic::AbcGeom::kVaryingScope))
   {
      TimeSample<Alembic::AbcGeom::IN3fGeomParam> sampler;
      Alembic::AbcGeom::IN3fGeomParam::Sample sample;
      
      size_t n = Nref.getNumSamples();
      
      if (n == 0)
      {
         return false;
      }
      
      double refTime = Nref.getTimeSampling()->getSampleTime(0);
      
      if (mDso->referenceSource() == RS_frame)
      {
         double minTime = refTime;
         double maxTime = Nref.getTimeSampling()->getSampleTime(n-1);
         
         refTime = mDso->referenceFrame() / mDso->fps();
         if (refTime < minTime) refTime = minTime;
         if (refTime > maxTime) refTime = maxTime;
      }
      
      if (!sampler.get(Nref, refTime))
      {
         return false;
      }
      
      sample = sampler.data();
      
      Alembic::Abc::N3fArraySamplePtr vals = sample.getVals();
      Alembic::Abc::UInt32ArraySamplePtr idxs = sample.getIndices();
      
      bool NrefPerPoint = (Nref.getScope() != Alembic::AbcGeom::kFacevaryingScope);
      bool validSize = false;
      
      if (idxs)
      {
         validSize = (NrefPerPoint ? idxs->size() == info.pointCount : idxs->size() == info.vertexCount);
      }
      else
      {
         validSize = (NrefPerPoint ? vals->size() == info.pointCount : vals->size() == info.vertexCount);
      }
      
      if (validSize)
      {
         float *vl = 0;
         unsigned int *il = 0;
         
         if (NrefPerPoint)
         {
            vl = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
         }
         else
         {
            vl = (float*) AiMalloc(3 * vals->size() * sizeof(float));
         }
         
         if (!NrefPerPoint || !idxs)
         {
            // Can copy vector values straight from alembic property
            for (size_t i=0, j=0; i<vals->size(); ++i, j+=3)
            {
               Alembic::Abc::N3f val;
               
               Mref.multDirMatrix(vals->get()[i], val);
               val.normalize();
               
               vl[j+0] = val.x;
               vl[j+1] = val.y;
               vl[j+2] = val.z;
            }
         }
         
         if (idxs)
         {
            if (!NrefPerPoint)
            {
               il = (unsigned int*) AiMalloc(info.vertexCount * sizeof(unsigned int));
               
               for (unsigned int i=0; i<info.vertexCount; ++i)
               {
                  il[info.arnoldVertexIndex[i]] = idxs->get()[i];
               }
            }
            else
            {
               // no mapping
               for (unsigned int i=0, j=0; i<info.pointCount; ++i, j+=3)
               {
                  Alembic::Abc::N3f val;
                  
                  Mref.multDirMatrix(vals->get()[idxs->get()[i]], val);
                  val.normalize();
                  
                  vl[j+0] = val.x;
                  vl[j+1] = val.y;
                  vl[j+2] = val.z;
               }
            }
         }
         else
         {
            if (!NrefPerPoint)
            {
               // Build straight mapping 
               il = (unsigned int*) AiMalloc(info.vertexCount * sizeof(unsigned int));
               
               for (unsigned int i=0; i<info.vertexCount; ++i)
               {
                  il[info.arnoldVertexIndex[i]] = i;
               }
            }
         }
         
         // Destroy existing values
         UserAttributes::iterator uait;
         
         uait = info.vertexAttrs.find(NrefName);
         if (uait != info.vertexAttrs.end())
         {
            DestroyUserAttribute(uait->second);
            info.vertexAttrs.erase(uait);
         }
         
         uait = info.pointAttrs.find(NrefName);
         if (uait != info.pointAttrs.end())
         {
            DestroyUserAttribute(uait->second);
            info.pointAttrs.erase(uait);
         }
         
         // Create new reference normals attribute of the right class
         UserAttribute &ua = (NrefPerPoint ? info.pointAttrs[NrefName] : info.vertexAttrs[NrefName]);
   
         InitUserAttribute(ua);
         
         ua.arnoldCategory = (NrefPerPoint ? AI_USERDEF_VARYING : AI_USERDEF_INDEXED);
         ua.arnoldType = AI_TYPE_VECTOR;
         ua.arnoldTypeStr = "VECTOR";
         ua.isArray = true;
         ua.dataDim = 3;
         ua.dataCount = (NrefPerPoint ? info.pointCount : vals->size());
         ua.data = vl;
         ua.indicesCount = (NrefPerPoint ? 0 : info.vertexCount);
         ua.indices = (NrefPerPoint ? 0 : il);
         
         return true;
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

AlembicNode::VisitReturn MakeShape::enter(AlembicMesh &node, AlembicNode *instance)
{
   Alembic::AbcGeom::IPolyMeshSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(node, instance);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   MeshInfo info;
   
   info.varyingTopology = (mDso->forceVelocityBlur() || schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology);
   
   if (schema.getUVsParam().valid())
   {
      info.UVs[""] = schema.getUVsParam();
   }
   
   // Collect attributes
   
   double attribsTime = (info.varyingTopology ? mDso->renderTime() : mDso->attributesTime(mDso->attributesEvaluationTime()));
   
   bool interpolateAttribs = !info.varyingTopology;
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         attribsTime,
                         interpolateAttribs,
                         &info.objectAttrs,
                         &info.primitiveAttrs,
                         &info.pointAttrs,
                         &info.vertexAttrs,
                         &info.UVs);
   
   // Create base mesh
   
   // Figure out if normals need to be read or computed 
   Alembic::AbcGeom::IN3fGeomParam N;
   std::vector<float*> _smoothNormals;
   std::vector<float*> *smoothNormals = 0;
   bool subd = false;
   bool smoothing = false;
   bool readNormals = false;
   bool NperPoint = false;
   
   const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mDso->procNode(), "subdiv_type");
   
   if (pe)
   {
      subd = (AiNodeGetInt(mDso->procNode(), "subdiv_type") > 0);
   }
   
   if (!subd)
   {
      pe = AiNodeLookUpUserParameter(mDso->procNode(), "smoothing");
      if (pe)
      {
         smoothing = AiNodeGetBool(mDso->procNode(), "smoothing");
      }
   }
   
   readNormals = (!subd && smoothing);
   
   if (readNormals)
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Read normals data if available");
      }
      
      N = schema.getNormalsParam();
      
      if (N.valid() && (N.getScope() == Alembic::AbcGeom::kFacevaryingScope ||
                        N.getScope() == Alembic::AbcGeom::kVertexScope ||
                        N.getScope() == Alembic::AbcGeom::kVaryingScope))
      {
         NperPoint = (N.getScope() != Alembic::AbcGeom::kFacevaryingScope);
      }
      else
      {
         smoothNormals = &_smoothNormals;
         NperPoint = true;
      }
   }
   
   mNode = generateBaseMesh(node, instance, info, smoothNormals);
   
   if (!mNode)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Output normals
   
   AtArray *nlist = 0;
   AtArray *nidxs = 0;
   
   if (readNormals)
   {
      // Arnold requires as many normal samples as position samples
      if (smoothNormals)
      {
         // Output smooth normals (per-point)
         // As normals are computed from positions, sample count will match
         
         // build vertex -> point mappings
         nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
         for (unsigned int k=0; k<info.vertexCount; ++k)
         {
            AiArraySetUInt(nidxs, k, info.vertexPointIndex[k]);
         }
         
         nlist = AiArrayAllocate(info.pointCount, smoothNormals->size(), AI_TYPE_VECTOR);
         
         unsigned int j = 0;
         AtVector n;
         
         for (std::vector<float*>::iterator nit=smoothNormals->begin(); nit!=smoothNormals->end(); ++nit)
         {
            float *N = *nit;
            
            for (unsigned int k=0; k<info.pointCount; ++k, ++j, N+=3)
            {
               n.x = N[0];
               n.y = N[1];
               n.z = N[2];
               
               AiArraySetVec(nlist, j, n);
            }
            
            AiFree(*nit);
         }
         
         smoothNormals->clear();
         
         AiNodeSetArray(mNode, "nlist", nlist);
         AiNodeSetArray(mNode, "nidxs", nidxs);
      }
      else
      {
         TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> Nsamples;
         
         if (info.varyingTopology)
         {
            Nsamples.update(N, mDso->renderTime(), mDso->renderTime(), false);
         }
         else
         {
            for (size_t i=0; i<mDso->numMotionSamples(); ++i)
            {
               double t = mDso->motionSampleTime(i);
            
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Sample normals \"%s\" at t=%lf", node.path().c_str(), t);
               }
            
               Nsamples.update(N, t, t, i>0);
            }
         }
         
         if (Nsamples.size() > 0)
         {
            TimeSampleList<Alembic::AbcGeom::IN3fGeomParam>::ConstIterator n0, n1;
            double blend = 0.0;
            AtVector vec;
            
            AtArray *vlist = AiNodeGetArray(mNode, "vlist");
            size_t requiredSamples = size_t(AiArrayGetNumKeys(vlist));
            
            // requiredSamples is either 1 or numMotionSamples()
            bool unexpectedSampleCount = (requiredSamples != 1 && requiredSamples != mDso->numMotionSamples());
            if (unexpectedSampleCount)
            {
               AiMsgWarning("[abcproc] Unexpected mesh sample count %lu.", requiredSamples);
            }
            
            if (info.varyingTopology || unexpectedSampleCount)
            {
               Nsamples.getSamples(mDso->renderTime(), n0, n1, blend);
               
               Alembic::Abc::N3fArraySamplePtr vals = n0->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs = n0->data().getIndices();
               
               nlist = AiArrayAllocate(vals->size(), requiredSamples, AI_TYPE_VECTOR);
               
               size_t ncount = vals->size();
               
               for (size_t i=0; i<ncount; ++i)
               {
                  Alembic::Abc::N3f val = vals->get()[i];
                  
                  vec.x = val.x;
                  vec.y = val.y;
                  vec.z = val.z;
                  
                  for (size_t j=0, k=0; j<requiredSamples; ++j, k+=ncount)
                  {
                     AiArraySetVec(nlist, i+k, vec);
                  }
               }
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu normals", vals->size());
               }
               
               if (NperPoint)
               {
                  if (idxs)
                  {
                     nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
                     
                     for (unsigned int i=0; i<info.vertexCount; ++i)
                     {
                        unsigned int vidx = info.arnoldVertexIndex[i];
                        unsigned int pidx = info.vertexPointIndex[vidx];
                        
                        if (pidx >= idxs->size())
                        {
                           AiMsgWarning("[abcproc] Invalid normal index");
                           AiArraySetUInt(nidxs, vidx, 0);
                        }
                        else
                        {
                           AiArraySetUInt(nidxs, vidx, idxs->get()[pidx]);
                        }
                     }
                  }
                  else
                  {
                     nidxs = AiArrayCopy(AiNodeGetArray(mNode, "vidxs"));
                  }
               }
               else
               {     
                  if (idxs)
                  {
                     nidxs = AiArrayAllocate(idxs->size(), 1, AI_TYPE_UINT);
                     
                     for (size_t i=0; i<idxs->size(); ++i)
                     {
                        AiArraySetUInt(nidxs, info.arnoldVertexIndex[i], idxs->get()[i]);
                     }
                     
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu normal indices", idxs->size());
                     }
                  }
                  else
                  {
                     nidxs = AiArrayAllocate(vals->size(), 1, AI_TYPE_UINT);
                     
                     for (size_t i=0; i<vals->size(); ++i)
                     {
                        AiArraySetUInt(nidxs, info.arnoldVertexIndex[i], i);
                     }
                  }
               }
            }
            else
            {
               for (size_t i=0, j=0; i<requiredSamples; ++i)
               {
                  double t = (requiredSamples == 1 ? mDso->renderTime() : mDso->motionSampleTime(i));
                  
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read normal samples [t=%lf]", t);
                  }
                  
                  Nsamples.getSamples(t, n0, n1, blend);
                  
                  if (!nlist)
                  {
                     nlist = AiArrayAllocate(info.vertexCount, requiredSamples, AI_TYPE_VECTOR);
                  }
                  
                  if (blend > 0.0)
                  {
                     Alembic::Abc::N3fArraySamplePtr vals0 = n0->data().getVals();
                     Alembic::Abc::N3fArraySamplePtr vals1 = n1->data().getVals();
                     Alembic::Abc::UInt32ArraySamplePtr idxs0 = n0->data().getIndices();
                     Alembic::Abc::UInt32ArraySamplePtr idxs1 = n1->data().getIndices();
                     
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu normals / %lu normals [interpolate]", vals0->size(), vals1->size());
                     }
                     
                     double a = 1.0 - blend;
                     
                     if (idxs0)
                     {
                        if (mDso->verbose())
                        {
                           AiMsgInfo("[abcproc] Read %lu normal indices", idxs0->size());
                        }
                        
                        if (idxs1)
                        {
                           if (mDso->verbose())
                           {
                              AiMsgInfo("[abcproc] Read %lu normal indices", idxs0->size());
                           }
                           
                           if (idxs0->size() != idxs1->size() ||
                               ( NperPoint && idxs0->size() != info.pointCount) ||
                               (!NperPoint && idxs0->size() != info.vertexCount))
                           {
                              if (nidxs) AiArrayDestroy(nidxs);
                              if (nlist) AiArrayDestroy(nlist);
                              nidxs = 0;
                              nlist = 0;
                              AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                              break;
                           }
                           
                           if (NperPoint)
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 unsigned int vidx = info.arnoldVertexIndex[k];
                                 unsigned int pidx = info.vertexPointIndex[vidx];
                                 
                                 if (pidx >= idxs0->size() || pidx >= idxs1->size())
                                 {
                                    AiMsgWarning("[abcproc] Invalid normal index");
                                    AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                                 }
                                 else
                                 {
                                    Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[pidx]];
                                    Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[pidx]];
                                    
                                    vec.x = a * val0.x + blend * val1.x;
                                    vec.y = a * val0.y + blend * val1.y;
                                    vec.z = a * val0.z + blend * val1.z;
                                    
                                    AiArraySetVec(nlist, j + vidx, vec);
                                 }
                              }
                           }
                           else
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[k]];
                                 Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[k]];
                                 
                                 vec.x = a * val0.x + blend * val1.x;
                                 vec.y = a * val0.y + blend * val1.y;
                                 vec.z = a * val0.z + blend * val1.z;
                                 
                                 AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                              }
                           }
                        }
                        else
                        {
                           if (vals1->size() != idxs0->size() ||
                               ( NperPoint && vals1->size() != info.pointCount) ||
                               (!NperPoint && vals1->size() != info.vertexCount))
                           {
                              if (nidxs) AiArrayDestroy(nidxs);
                              if (nlist) AiArrayDestroy(nlist);
                              nidxs = 0;
                              nlist = 0;
                              AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                              break;
                           }
                           
                           if (NperPoint)
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 unsigned int vidx = info.arnoldVertexIndex[k];
                                 unsigned int pidx = info.vertexPointIndex[vidx];
                                 
                                 if (pidx >= vals1->size() || pidx >= idxs0->size())
                                 {
                                    AiMsgWarning("[abcproc] Invalid normal index");
                                    AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                                 }
                                 else
                                 {
                                    Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[pidx]];
                                    Alembic::Abc::N3f val1 = vals1->get()[pidx];
                                    
                                    vec.x = a * val0.x + blend * val1.x;
                                    vec.y = a * val0.y + blend * val1.y;
                                    vec.z = a * val0.z + blend * val1.z;
                                    
                                    AiArraySetVec(nlist, j + vidx, vec);
                                 }
                              }
                           }
                           else
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[k]];
                                 Alembic::Abc::N3f val1 = vals1->get()[k];
                                 
                                 vec.x = a * val0.x + blend * val1.x;
                                 vec.y = a * val0.y + blend * val1.y;
                                 vec.z = a * val0.z + blend * val1.z;
                                 
                                 AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                              }
                           }
                        }
                     }
                     else
                     {
                        if (idxs1)
                        {
                           if (mDso->verbose())
                           {
                              AiMsgInfo("[abcproc] Read %lu normal indices", idxs1->size());
                           }
                           
                           if (vals0->size() != idxs1->size() ||
                               ( NperPoint && vals0->size() != info.pointCount) ||
                               (!NperPoint && vals0->size() != info.vertexCount))
                           {
                              if (nidxs) AiArrayDestroy(nidxs);
                              if (nlist) AiArrayDestroy(nlist);
                              nidxs = 0;
                              nlist = 0;
                              AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                              break;
                           }
                           
                           if (NperPoint)
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 unsigned int vidx = info.arnoldVertexIndex[k];
                                 unsigned int pidx = info.vertexPointIndex[vidx];
                                 
                                 if (pidx >= vals0->size() || pidx >= idxs1->size())
                                 {
                                    AiMsgWarning("[abcproc] Invalid normal index");
                                    AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                                 }
                                 else
                                 {
                                    Alembic::Abc::N3f val0 = vals0->get()[pidx];
                                    Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[pidx]];
                                    
                                    vec.x = a * val0.x + blend * val1.x;
                                    vec.y = a * val0.y + blend * val1.y;
                                    vec.z = a * val0.z + blend * val1.z;
                                    
                                    AiArraySetVec(nlist, j + vidx, vec);
                                 }
                              }
                           }
                           else
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 Alembic::Abc::N3f val0 = vals0->get()[k];
                                 Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[k]];
                                 
                                 vec.x = a * val0.x + blend * val1.x;
                                 vec.y = a * val0.y + blend * val1.y;
                                 vec.z = a * val0.z + blend * val1.z;
                                 
                                 AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                              }
                           }
                        }
                        else
                        {
                           if (vals1->size() != vals0->size() ||
                               ( NperPoint && vals0->size() != info.pointCount) ||
                               (!NperPoint && vals0->size() != info.vertexCount))
                           {
                              if (nidxs) AiArrayDestroy(nidxs);
                              if (nlist) AiArrayDestroy(nlist);
                              nidxs = 0;
                              nlist = 0;
                              AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                              break;
                           }
                           
                           if (NperPoint)
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 unsigned int vidx = info.arnoldVertexIndex[k];
                                 unsigned int pidx = info.vertexPointIndex[vidx];
                                 
                                 if (pidx >= vals0->size())
                                 {
                                    AiMsgWarning("[abcproc] Invalid normal index");
                                    AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                                 }
                                 else
                                 {
                                    Alembic::Abc::N3f val0 = vals0->get()[pidx];
                                    Alembic::Abc::N3f val1 = vals1->get()[pidx];
                                    
                                    vec.x = a * val0.x + blend * val1.x;
                                    vec.y = a * val0.y + blend * val1.y;
                                    vec.z = a * val0.z + blend * val1.z;
                                    
                                    AiArraySetVec(nlist, j + vidx, vec);
                                 }
                              }
                           }
                           else
                           {
                              for (unsigned int k=0; k<info.vertexCount; ++k)
                              {
                                 Alembic::Abc::N3f val0 = vals0->get()[k];
                                 Alembic::Abc::N3f val1 = vals1->get()[k];
                                 
                                 vec.x = a * val0.x + blend * val1.x;
                                 vec.y = a * val0.y + blend * val1.y;
                                 vec.z = a * val0.z + blend * val1.z;
                                 
                                 AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                              }
                           }
                        }
                     }
                  }
                  else
                  {
                     Alembic::Abc::N3fArraySamplePtr vals = n0->data().getVals();
                     Alembic::Abc::UInt32ArraySamplePtr idxs = n0->data().getIndices();
                     
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu normals", vals->size());
                     }
                     
                     if (idxs)
                     {
                        if (mDso->verbose())
                        {
                           AiMsgInfo("[abcproc] Read %lu normal indices", idxs->size());
                        }
                        
                        if (( NperPoint && idxs->size() != info.pointCount ) ||
                            (!NperPoint && idxs->size() != info.vertexCount))
                        {
                           AiArrayDestroy(nidxs);
                           AiArrayDestroy(nlist);
                           nidxs = 0;
                           nlist = 0;
                           AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                           break;
                        }
                        
                        if (NperPoint)
                        {
                           for (unsigned int k=0; k<info.vertexCount; ++k)
                           {
                              unsigned int vidx = info.arnoldVertexIndex[k];
                              unsigned int pidx = info.vertexPointIndex[vidx];
                              
                              if (pidx >= idxs->size())
                              {
                                 AiMsgWarning("[abcproc] Invalid normal index");
                                 AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                              }
                              else
                              {
                                 Alembic::Abc::N3f val = vals->get()[idxs->get()[pidx]];
                                 
                                 vec.x = val.x;
                                 vec.y = val.y;
                                 vec.z = val.z;
                                 
                                 AiArraySetVec(nlist, j + vidx, vec);
                              }
                           }
                        }
                        else
                        {
                           for (unsigned int k=0; k<info.vertexCount; ++k)
                           {
                              Alembic::Abc::N3f val = vals->get()[idxs->get()[k]];
                              
                              vec.x = val.x;
                              vec.y = val.y;
                              vec.z = val.z;
                              
                              AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                           }
                        }
                     }
                     else
                     {
                        if (( NperPoint && vals->size() != info.pointCount ) ||
                            (!NperPoint && vals->size() != info.vertexCount))
                        {
                           AiArrayDestroy(nidxs);
                           if (nlist) AiArrayDestroy(nlist);
                           nidxs = 0;
                           nlist = 0;
                           AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                           break;
                        }
                        
                        if (NperPoint)
                        {
                           for (unsigned int k=0; k<info.vertexCount; ++k)
                           {
                              unsigned int vidx = info.arnoldVertexIndex[k];
                              unsigned int pidx = info.vertexPointIndex[vidx];
                              
                              if (pidx >= vals->size())
                              {
                                 AiMsgWarning("[abcproc] Invalid normal index");
                                 AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                              }
                              else
                              {
                                 Alembic::Abc::N3f val = vals->get()[pidx];
                              
                                 vec.x = val.x;
                                 vec.y = val.y;
                                 vec.z = val.z;
                                 
                                 AiArraySetVec(nlist, j + vidx, vec);
                              }
                           }
                        }
                        else
                        {
                           for (unsigned int k=0; k<info.vertexCount; ++k)
                           {
                              Alembic::Abc::N3f val = vals->get()[k];
                              
                              vec.x = val.x;
                              vec.y = val.y;
                              vec.z = val.z;
                              
                              AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                           }
                        }
                     }
                  }
                  
                  if (!nidxs)
                  {
                     // vertex remapping already happened at values level
                     nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
                     
                     for (unsigned int k=0; k<info.vertexCount; ++k)
                     {
                        AiArraySetUInt(nidxs, k, k);
                     }
                  }
                  
                  j += info.vertexCount;
               }
            }
            
            if (nlist && nidxs)
            {
               AiNodeSetArray(mNode, "nlist", nlist);
               AiNodeSetArray(mNode, "nidxs", nidxs);
            }
            else
            {
               AiMsgWarning("[abcproc] Ignore normals: No valid data");
            }
         }
      }
   }
   
   // Output UVs
   
   outputMeshUVs(node, mNode, info);
   
   // Output reference (Pref)
   
   if (mDso->outputReference())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate reference attributes");
      }
      
      AlembicMesh *refMesh = 0;
      
      bool hasPref = getReferenceMesh(node, info, refMesh);
      // Note: if hasPref is true, refMesh is null
      
      if (hasPref || refMesh)
      {
         // World matrix to apply to points from reference (applies to both P and N)
         Alembic::Abc::M44d Mref;
         
         // fillReferencePositions may fail if data doesn't fit
         hasPref = fillReferencePositions(refMesh, info, Mref);
         
         // Now we are guarantied to have a Pref attribute in info.pointAttrs
         
         // Generate Nref
         
         if (hasPref && readNormals)
         {
            bool hasNref = false;
            
            if (refMesh)
            {
               hasNref = readReferenceNormals(refMesh, info, Mref);
            }
            else
            {
               hasNref = checkReferenceNormals(info);
            }
            
            if (!hasNref)
            {
               // Generate Nref from Pref
               const std::string &NrefName = mDso->referenceNormalName();
               const std::string &PrefName = mDso->referencePositionName();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Compute smooth \"%s\" from \"%s\"", NrefName.c_str(), PrefName.c_str());
               }
               
               UserAttribute &ua = info.pointAttrs[NrefName];
               
               InitUserAttribute(ua);
                        
               ua.arnoldCategory = AI_USERDEF_VARYING;
               ua.arnoldType = AI_TYPE_VECTOR;
               ua.arnoldTypeStr = "VECTOR";
               ua.isArray = true;
               ua.dataDim = 3;
               ua.dataCount = info.pointCount;
               ua.data = computeMeshSmoothNormals(info, (const float*) info.pointAttrs[PrefName].data, 0, 0.0f);
            }
         }
         
         // Tref?
         // Bref?
      }
      
      if (!hasPref)
      {
         AiMsgWarning("[abcproc] Invalid reference object specification (%s).", mDso->objectPath().c_str());
      }
   }
   
   // Output user defined attributes
   
   removeConflictingAttribs(mNode, &(info.objectAttrs), &(info.primitiveAttrs), &(info.pointAttrs), &(info.vertexAttrs), mDso->verbose());
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.primitiveAttrs, info.polygonCount);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
   SetUserAttributes(mNode, info.vertexAttrs, info.vertexCount, info.arnoldVertexIndex);
   
   // Make sure we have the 'instance_num' attribute
   
   outputInstanceNumber(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicSubD &node, AlembicNode *instance)
{
   // generate a polymesh
   Alembic::AbcGeom::ISubDSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(node, instance);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   MeshInfo info;
   
   info.varyingTopology = (mDso->forceVelocityBlur() || schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology);
   
   if (schema.getUVsParam().valid())
   {
      info.UVs[""] = schema.getUVsParam();
   }
   
   // Collect attributes
   
   double attribsTime = (info.varyingTopology ? mDso->renderTime() : mDso->attributesTime(mDso->attributesEvaluationTime()));
   
   bool interpolateAttribs = !info.varyingTopology;
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         attribsTime,
                         interpolateAttribs,
                         &info.objectAttrs,
                         &info.primitiveAttrs,
                         &info.pointAttrs,
                         &info.vertexAttrs,
                         &info.UVs);
   
   // Create base mesh
   
   // Check if normals should be computed
   std::vector<float*> _smoothNormals;
   std::vector<float*> *smoothNormals = 0;
   bool subd = false;
   bool smoothing = false;
   bool readNormals = false;
   
   const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mDso->procNode(), "subdiv_type");
   
   if (pe)
   {
      subd = (AiNodeGetInt(mDso->procNode(), "subdiv_type") > 0);
   }
   
   if (!subd)
   {
      pe = AiNodeLookUpUserParameter(mDso->procNode(), "smoothing");
      if (pe)
      {
         smoothing = AiNodeGetBool(mDso->procNode(), "smoothing");
      }
   }
   
   readNormals = (!subd && smoothing);
   
   if (readNormals)
   {
      smoothNormals = &_smoothNormals;
   }
   
   mNode = generateBaseMesh(node, instance, info, smoothNormals);
   
   if (!mNode)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Output normals
   
   if (readNormals)
   {
      AtArray *nlist = 0;
      AtArray *nidxs = 0;
      
      // output smooth normals (per-point)
      
      // build vertex -> point mappings
      nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
      for (unsigned int k=0; k<info.vertexCount; ++k)
      {
         AiArraySetUInt(nidxs, k, info.vertexPointIndex[k]);
      }
      
      nlist = AiArrayAllocate(info.pointCount, smoothNormals->size(), AI_TYPE_VECTOR);
      
      unsigned int j = 0;
      AtVector n;
      
      for (std::vector<float*>::iterator nit=smoothNormals->begin(); nit!=smoothNormals->end(); ++nit)
      {
         float *N = *nit;
         
         for (unsigned int k=0; k<info.pointCount; ++k, ++j, N+=3)
         {
            n.x = N[0];
            n.y = N[1];
            n.z = N[2];
            
            AiArraySetVec(nlist, j, n);
         }
         
         AiFree(*nit);
      }
      
      smoothNormals->clear();
      
      AiNodeSetArray(mNode, "nlist", nlist);
      AiNodeSetArray(mNode, "nidxs", nidxs);
   }
   
   // Output UVs
   
   outputMeshUVs(node, mNode, info);
   
   // Output referece
   
   if (mDso->outputReference())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate reference attributes");
      }
      
      AlembicSubD *refMesh = 0;
      
      bool hasPref = getReferenceMesh(node, info, refMesh);
      
      if (hasPref || refMesh)
      {
         // Generate Pref
         
         // World matrix to apply to points from reference (applies to both P and N)
         Alembic::Abc::M44d Mref;
         
         hasPref = fillReferencePositions(refMesh, info, Mref);
         
         // Generate Nref
         if (hasPref && readNormals)
         {
            // Check for attribute if Pref is also coming from an attribute
            bool hasNref = (!refMesh && checkReferenceNormals(info));
            
            if (!hasNref)
            {
               // Generate Nref from Pref
               const std::string &NrefName = mDso->referenceNormalName();
               const std::string &PrefName = mDso->referencePositionName();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Compute smooth \"%s\" from \"%s\"", NrefName.c_str(), PrefName.c_str());
               }
               
               UserAttributes::iterator uait;
               
               uait = info.vertexAttrs.find(NrefName);
               if (uait != info.vertexAttrs.end())
               {
                  DestroyUserAttribute(uait->second);
                  info.vertexAttrs.erase(uait);
               }
               
               uait = info.pointAttrs.find(NrefName);
               if (uait != info.pointAttrs.end())
               {
                  DestroyUserAttribute(uait->second);
                  info.pointAttrs.erase(uait);
               }
               
               UserAttribute &ua = info.pointAttrs[NrefName];
               
               InitUserAttribute(ua);
                        
               ua.arnoldCategory = AI_USERDEF_VARYING;
               ua.arnoldType = AI_TYPE_VECTOR;
               ua.arnoldTypeStr = "VECTOR";
               ua.isArray = true;
               ua.dataDim = 3;
               ua.dataCount = info.pointCount;
               ua.data = computeMeshSmoothNormals(info, (const float*) info.pointAttrs[PrefName].data, 0, 0.0f);
            }
         }
         
         // Tref?
         // Bref?
      }
      
      if (!hasPref)
      {
         AiMsgWarning("[abcproc] Invalid reference object specification (%s).", mDso->objectPath().c_str());
      }
   }
   
   // Output user defined attributes
   
   removeConflictingAttribs(mNode, &(info.objectAttrs), &(info.primitiveAttrs), &(info.pointAttrs), &(info.vertexAttrs), mDso->verbose());
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.primitiveAttrs, info.polygonCount);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
   SetUserAttributes(mNode, info.vertexAttrs, info.vertexCount, info.arnoldVertexIndex);
   
   // Make sure we have the 'instance_num' attribute
   
   outputInstanceNumber(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicPoints &node, AlembicNode *instance)
{
   Alembic::AbcGeom::IPointsSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(node, instance);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   PointsInfo info;
   UserAttributes extraPointAttrs;
   
   // Generate base points
   
   TimeSampleList<Alembic::AbcGeom::IPointsSchema> &samples = node.samples().schemaSamples;
   TimeSampleList<Alembic::AbcGeom::IPointsSchema>::ConstIterator samp0, samp1;
   double a = 1.0;
   double b = 0.0;
   
   for (size_t i=0; i<mDso->numMotionSamples(); ++i)
   {
      double t = mDso->motionSampleTime(i);
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Sample points \"%s\" at t=%lf", node.path().c_str(), t);
      }
   
      node.sampleSchema(t, t, i > 0);
   }
   
   // renderTime() may not be in the sample list, let's at it just in case
   node.sampleSchema(mDso->renderTime(), mDso->renderTime(), true);
   
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Read %lu points samples", samples.size());
   }
   
   if (samples.size() == 0 ||
       !samples.getSamples(mDso->renderTime(), samp0, samp1, b))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   mNode = createArnoldNode(Strings::points, (instance ? *instance : node), true);
   
   // Build IDmap
   Alembic::Abc::P3fArraySamplePtr P0 = samp0->data().getPositions();
   Alembic::Abc::V3fArraySamplePtr V0 = samp0->data().getVelocities();
   Alembic::Abc::UInt64ArraySamplePtr ID0 = samp0->data().getIds();
   
   Alembic::Abc::P3fArraySamplePtr P1;
   Alembic::Abc::UInt64ArraySamplePtr ID1;
   Alembic::Abc::V3fArraySamplePtr V1;
   
   std::map<Alembic::Util::uint64_t, size_t> idmap0; // ID -> index in P0/ID0/V0
   std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> > idmap1; // ID -> (index in P1/ID1/V1, index in final point list)
   std::map<Alembic::Util::uint64_t, size_t> sharedids; // ID -> index in P1/ID1/V1
   
   // Collect attributes
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         samp0->time(),
                         false,
                         &info.objectAttrs,
                         0,
                         &info.pointAttrs,
                         0,
                         0);
   
   const float *vel0 = 0;
   const float *vel1 = 0;
   const float *acc0 = 0;
   const float *acc1 = 0;
   
   for (size_t i=0; i<ID0->size(); ++i)
   {
      idmap0[ID0->get()[i]] = i;
   }
   
   info.pointCount = idmap0.size();
   
   // Get velocities and accelerations
   std::string vname, aname;
   UserAttributes::iterator it= info.pointAttrs.end();
   
   const char *velName = mDso->velocityName();
   if (velName)
   {
      if (strcmp(velName, "<builtin>") != 0)
      {
         it = info.pointAttrs.find(velName);
         if (it != info.pointAttrs.end() &&
             (it->second.arnoldType != AI_TYPE_VECTOR ||
             it->second.dataCount != info.pointCount))
         {
            it = info.pointAttrs.end();
         }
      }
   }
   else
   {
      it = info.pointAttrs.find("velocity");
      if (it == info.pointAttrs.end() ||
          it->second.arnoldType != AI_TYPE_VECTOR ||
          it->second.dataCount != info.pointCount)
      {
         it = info.pointAttrs.find("vel");
         if (it == info.pointAttrs.end() ||
             it->second.arnoldType != AI_TYPE_VECTOR ||
             it->second.dataCount != info.pointCount)
         {
            it = info.pointAttrs.find("v");
            if (it != info.pointAttrs.end() &&
                (it->second.arnoldType != AI_TYPE_VECTOR ||
                it->second.dataCount != info.pointCount))
            {
               it = info.pointAttrs.end();
            }
         }
      }
   }
   
   if (it != info.pointAttrs.end())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Using user defined attribute \"%s\" for point velocities", it->first.c_str());
      }
      vname = it->first;
      vel0 = (const float*) it->second.data;
   }
   else if (V0)
   {
      if (V0->size() != P0->size())
      {
         AiMsgWarning("[abcproc] (1) Velocities count doesn't match points' one (%lu for %lu). Ignoring it.", V0->size(), P0->size());
      }
      else
      {
         vel0 = (const float*) V0->getData();
      }
   }
   
   if (vel0)
   {
      const char *accName = mDso->accelerationName();
      
      if (accName)
      {
         it = info.pointAttrs.find(accName);
         if (it != info.pointAttrs.end() &&
             (it->second.arnoldType != AI_TYPE_VECTOR ||
              it->second.dataCount != info.pointCount))
         {
            it = info.pointAttrs.end();
         }
      }
      else
      {
         it = info.pointAttrs.find("acceleration");
         if (it == info.pointAttrs.end() ||
             it->second.arnoldType != AI_TYPE_VECTOR ||
             it->second.dataCount != info.pointCount)
         {
            it = info.pointAttrs.find("accel");
            if (it == info.pointAttrs.end() ||
                it->second.arnoldType != AI_TYPE_VECTOR ||
                it->second.dataCount != info.pointCount)
            {
               it = info.pointAttrs.find("a");
               if (it != info.pointAttrs.end() &&
                   (it->second.arnoldType != AI_TYPE_VECTOR ||
                    it->second.dataCount != info.pointCount))
               {
                  it = info.pointAttrs.end();
               }
            }
         }
      }
      
      if (it != info.pointAttrs.end())
      {
         aname = it->first;
         acc0 = (const float*) it->second.data;
      }
   }
   
   if (b > 0.0)
   {
      P1 = samp1->data().getPositions();
      ID1 = samp1->data().getIds();
      V1 = samp1->data().getVelocities();
      
      a = 1.0 - b;
      
      for (size_t i=0; i<ID1->size(); ++i)
      {
         Alembic::Util::uint64_t id = ID1->get()[i];
         
         if (idmap0.find(id) != idmap0.end())
         {
            sharedids[id] = i;
         }
         else
         {
            std::pair<size_t, size_t> idxs;
            
            idxs.first = i;
            idxs.second = info.pointCount++;
            
            idmap1[id] = idxs;
         }
      }
      
      // Collect point attributes
      
      collectUserAttributes(Alembic::Abc::ICompoundProperty(), schema.getArbGeomParams(),
                            samp1->time(), false, 0, 0, &extraPointAttrs, 0, 0);
      
      // Get velocities and accelerations
      
      if (vname.length() > 0)
      {
         it = extraPointAttrs.find(vname);
         if (it != extraPointAttrs.end() && it->second.dataCount == P1->size())
         {
            vel1 = (const float*) it->second.data;
         }
      }
      else if (V1)
      {
         if (V1->size() != P1->size())
         {
            AiMsgWarning("[abcproc] (2) Velocities count doesn't match points' one (%lu for %lu). Ignoring it.", V1->size(), P1->size());
         }
         else
         {
            vel1 = (const float*) V1->getData();
         }
      }
      
      if (aname.length() > 0)
      {
         it = extraPointAttrs.find(aname);
         if (it != extraPointAttrs.end() && it->second.dataCount == P1->size())
         {
            acc1 = (const float*) it->second.data;
         }
      }
   }
   
   // Fill points
   
   unsigned int nkeys = (vel0 ? mDso->numMotionSamples() : 1);
   
   AtArray *points = AiArrayAllocate(info.pointCount, nkeys, AI_TYPE_VECTOR);
   
   std::map<Alembic::Util::uint64_t, size_t>::iterator idit;
   AtVector pnt;
   
   for (unsigned int i=0, koff=0; i<nkeys; ++i, koff+=info.pointCount)
   {
      double t = (vel0 ? mDso->motionSampleTime(i) : mDso->renderTime());
      
      for (size_t j=0, voff=0; j<P0->size(); ++j, voff+=3)
      {
         Alembic::Util::uint64_t id = ID0->get()[j];
         
         idit = sharedids.find(id);
         
         if (idit != sharedids.end())
         {
            Alembic::Abc::V3f P = float(a) * P0->get()[j] + float(b) * P1->get()[idit->second];
            
            pnt.x = P.x;
            pnt.y = P.y;
            pnt.z = P.z;
            
            if (vel0 && vel1)
            {
               // Note: a * samp0->time() + b * samp1->time() == renderTime
               double dt = t - mDso->renderTime();
               dt *= mDso->velocityScale();
               
               size_t off = idit->second * 3;
               
               pnt.x += dt * (a * vel0[voff  ] + b * vel1[off  ]);
               pnt.y += dt * (a * vel0[voff+1] + b * vel1[off+1]);
               pnt.z += dt * (a * vel0[voff+2] + b * vel1[off+2]);
               
               if (acc0 && acc1)
               {
                  double hdt2 = 0.5 * dt * dt;
                  
                  pnt.x += hdt2 * (a * acc0[voff  ] + b * acc1[off  ]);
                  pnt.y += hdt2 * (a * acc0[voff+1] + b * acc1[off+1]);
                  pnt.z += hdt2 * (a * acc0[voff+2] + b * acc1[off+2]);
               }
               
               // Note: should adjust velocity and acceleration attribute values in final output too
            }
         }
         else
         {
            Alembic::Abc::V3f P = P0->get()[j];
            
            pnt.x = P.x;
            pnt.y = P.y;
            pnt.z = P.z;
            
            if (vel0)
            {
               double dt = t - samp0->time();
               dt *= mDso->velocityScale();
               
               pnt.x += dt * vel0[voff  ];
               pnt.y += dt * vel0[voff+1];
               pnt.z += dt * vel0[voff+2];
               
               if (acc0)
               {
                  double hdt2 = 0.5 * dt * dt;
                  
                  pnt.x += hdt2 * acc0[voff  ];
                  pnt.y += hdt2 * acc0[voff+1];
                  pnt.z += hdt2 * acc0[voff+2];
               }
            }
         }
         
         AiArraySetVec(points, koff+j, pnt);
      }
      
      std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator it;
      
      for (it=idmap1.begin(); it!=idmap1.end(); ++it)
      {
         Alembic::Abc::V3f P = P1->get()[it->second.first];
         
         pnt.x = P.x;
         pnt.y = P.y;
         pnt.z = P.z;
         
         if (vel1)
         {
            double dt = t - samp1->time();
            dt *= mDso->velocityScale();
            
            unsigned int voff = 3 * it->second.first;
            
            pnt.x += dt * vel1[voff  ];
            pnt.y += dt * vel1[voff+1];
            pnt.z += dt * vel1[voff+2];
            
            if (acc1)
            {
               double hdt2 = 0.5 * dt * dt;
               
               pnt.x += hdt2 * acc1[voff  ];
               pnt.y += hdt2 * acc1[voff+1];
               pnt.z += hdt2 * acc1[voff+2];
            }
         }
         
         AiArraySetVec(points, koff + it->second.second, pnt);
      }
   }
   
   AiNodeSetArray(mNode, "points", points);
   
   // mode: disk|sphere|quad
   // radius[]
   // aspect[]    (only for quad)
   // rotation[]  (only for quad)
   //
   // Use alternative attribtues for mode, aspect, rotation and radius?
   //
   // radius is handled by Alembic as 'widths' property
   // give priority to user defined 'radius' 
   bool radiusSet = false;
   
   const char *radiusName = mDso->radiusName();
   bool checkAltName = false;
   
   if (radiusName == 0)
   {
      radiusName = "radius";
   }
   
   Alembic::AbcGeom::IFloatGeomParam widths = schema.getWidthsParam();
   if (widths.valid())
   {
      // Convert to point attribute 'radius'
      
      if (info.pointAttrs.find(radiusName) != info.pointAttrs.end() && !mDso->forceConstantAttribute(radiusName))
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Ignore alembic \"widths\" property: use \"%s\" attribute instead.", radiusName);
         }
      }
      else if (checkAltName && info.pointAttrs.find("size") != info.pointAttrs.end() && !mDso->forceConstantAttribute("size"))
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Ignore alembic \"widths\" property: use \"size\" attribute instead.");
         }
      }
      else
      {
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam> wsamples;
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam>::ConstIterator wsamp0, wsamp1;
         
         double br = 0.0;
         
         if (!wsamples.update(widths, mDso->renderTime(), mDso->renderTime(), false))
         {
            AiMsgWarning("[abcproc] Could not read alembic points \"widths\" property for t = %lf", mDso->renderTime());
            // default to 1?
         }
         else if (!wsamples.getSamples(mDso->renderTime(), wsamp0, wsamp1, br))
         {
            AiMsgWarning("[abcproc] Could not read alembic points \"widths\" property for t = %lf", mDso->renderTime());
            // default to 1?
         }
         else
         {
            Alembic::Abc::FloatArraySamplePtr R0 = wsamp0->data().getVals();
            Alembic::Abc::FloatArraySamplePtr R1;
            
            if (wsamp0->time() != samp0->time())
            {
               AiMsgWarning("[abcproc] \"widths\" property sample time doesn't match points schema sample time (%lf and %lf respectively)",
                            samp0->time(), wsamp0->time());
            }
            else if (R0->size() != P0->size())
            {
               AiMsgWarning("[abcproc] \"widths\" property size doesn't match render time point count");
            }
            else
            {
               bool process = true;
               
               if (br > 0.0)
               {
                  R1 = wsamp1->data().getVals();
                  
                  if (wsamp1->time() != samp1->time())
                  {
                     AiMsgWarning("[abcproc] \"widths\" property sample time doesn't match points schema sample time (%lf and %lf respectively)",
                                  samp1->time(), wsamp1->time());
                     process = false;
                  }
                  else if (R1->size() != P1->size())
                  {
                     AiMsgWarning("[abcproc] \"widths\" property size doesn't match render time point count");
                     process = false;
                  }
               }
               
               if (process)
               {
                  float *radius = (float*) AiMalloc(info.pointCount * sizeof(float));
                  float r;
                  
                  if (R1)
                  {
                     for (size_t i=0; i<R0->size(); ++i)
                     {
                        Alembic::Util::uint64_t id = ID0->get()[i];
                        
                        idit = sharedids.find(id);
                        
                        if (idit != sharedids.end())
                        {
                           r = (1 - br) * R0->get()[i] + br * R1->get()[idit->second];
                        }
                        else
                        {
                           r = R0->get()[i];
                        }
                        
                        radius[i] = adjustRadius(r);
                     }
                     
                     std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator idit1;
                     
                     for (idit1 = idmap1.begin(); idit1 != idmap1.end(); ++idit1)
                     {
                        r = R1->get()[idit1->second.first];
                        
                        radius[idit1->second.second] = adjustRadius(r);
                     }
                  }
                  else
                  {
                     // asset(R0->size() == info.pointCount);
                     
                     for (size_t i=0; i<R0->size(); ++i)
                     {
                        r = R0->get()[i];
                        
                        radius[i] = adjustRadius(r);
                     }
                  }
                  
                  AiNodeSetArray(mNode, "radius", AiArrayConvert(info.pointCount, 1, AI_TYPE_FLOAT, radius));
                  
                  AiFree(radius);
                  
                  radiusSet = true;
               }
            }
         }
      }
   }
   
   // Output user defined attributes
   
   // Extend existing attributes
   
   if (idmap1.size() > 0)
   {
      for (UserAttributes::iterator it0 = info.pointAttrs.begin(); it0 != info.pointAttrs.end(); ++it0)
      {
         ResizeUserAttribute(it0->second, info.pointCount);
         
         UserAttributes::iterator it1 = extraPointAttrs.find(it0->first);
         
         if (it1 != extraPointAttrs.end())
         {
            std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator idit;
            
            for (idit = idmap1.begin(); idit != idmap1.end(); ++idit)
            {
               if (!CopyUserAttribute(it1->second, idit->second.first, 1,
                                      it0->second, idit->second.second))
               {
                  AiMsgWarning("[abcproc] Failed to copy extended user attribute data");
               }
            }
         }
      }
   }
   
   // Adjust radiuses
   if (!radiusSet)
   {
      UserAttribute *ra = 0;
      UserAttributes *attrs = 0;
      float constantRadius = -1.0f;
      
      UserAttributes::iterator ait = info.pointAttrs.find(radiusName);
      
      if (ait != info.pointAttrs.end())
      {
         if (mDso->forceConstantAttribute(radiusName))
         {
            if (ait->second.dataCount >= 1)
            {
               constantRadius = *((const float*) ait->second.data);
            }
            DestroyUserAttribute(ait->second);
            info.pointAttrs.erase(ait);
         }
         else
         {
            ra = &(ait->second);
            attrs = &(info.pointAttrs);
         }
      }
      
      if (!ra && checkAltName)
      {
         ait = info.pointAttrs.find("size");
         
         if (ait != info.pointAttrs.end())
         {
            if (mDso->forceConstantAttribute("size"))
            {
               if (ait->second.dataCount >= 1 && constantRadius < 0.0f)
               {
                  constantRadius = *((const float*) ait->second.data);
               }
               DestroyUserAttribute(ait->second);
               info.pointAttrs.erase(ait);
            }
            else
            {
               ra = &(ait->second);
               attrs = &(info.pointAttrs);
            }
         }
      }
      
      if (!ra && constantRadius < 0.0f)
      {
         ait = info.objectAttrs.find(radiusName);
         
         if (ait != info.objectAttrs.end())
         {
            ra = &(ait->second);
            attrs = &(info.objectAttrs);
         }
         else if (checkAltName)
         {
            ait = info.objectAttrs.find("size");
            
            if (ait != info.objectAttrs.end())
            {
               ra = &(ait->second);
               attrs = &(info.objectAttrs);
            }
         }
      }
      
      if (ra && ra->data && ra->arnoldType == AI_TYPE_FLOAT)
      {
         float *r = (float*) ra->data;
            
         for (unsigned int i=0; i<ra->dataCount; ++i)
         {
            for (unsigned int j=0; j<ra->dataDim; ++j, ++r)
            {
               *r = adjustRadius(*r);
            }
         }
         
         AtArray *radius = AiArrayConvert(ra->dataCount, 1, AI_TYPE_FLOAT, ra->data);
         
         AiNodeSetArray(mNode, "radius", radius);
         
         DestroyUserAttribute(ait->second);
         attrs->erase(ait);
      }
      else
      {
         AtArray *radius = AiArrayAllocate(1, 1, AI_TYPE_FLOAT);
         
         if (constantRadius < 0.0f)
         {
            AiMsgInfo("[abcproc] No radius set for points in alembic archive. Create particles with constant radius %lf (can be changed using dso '-radiusmin' data flag or 'abc_radiusmin' user attribute)", mDso->radiusMin());
            AiArraySetFlt(radius, 0, mDso->radiusMin());
         }
         else
         {
            AiArraySetFlt(radius, 0, adjustRadius(constantRadius));
         }
         
         AiNodeSetArray(mNode, "radius", radius);
      }
   }
   
   // Note: arnold want particle point attributes as uniform attributes, not varying
   for (it = info.pointAttrs.begin(); it != info.pointAttrs.end(); ++it)
   {
      it->second.arnoldCategory = AI_USERDEF_UNIFORM;
   }
   
   removeConflictingAttribs(mNode, &(info.objectAttrs), 0, &(info.pointAttrs), 0, mDso->verbose());
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
   
   // Make sure we have the 'instance_num' attribute
   
   outputInstanceNumber(node, instance);
   
   return AlembicNode::ContinueVisit;
}

bool MakeShape::getReferenceCurves(AlembicCurves &node,
                                   CurvesInfo &info,
                                   AlembicCurves* &refCurves)
{
   ReferenceSource refSrc = mDso->referenceSource();
   bool hasPref = false;
   const std::string &PrefName = mDso->referencePositionName();
   
   if (refSrc == RS_attributes || refSrc == RS_attributes_then_file)
   {
      UserAttributes::iterator uait;
      
      uait = info.pointAttrs.find(PrefName);
      
      if (uait != info.pointAttrs.end())
      {
         if (isVaryingFloat3(info, uait->second))
         {
            hasPref = true;
         }
         else
         {
            // Pref exists but doesn't match requirements, get rid of it
            DestroyUserAttribute(uait->second);
            info.pointAttrs.erase(uait);
         }
      }
   }
   
   if (hasPref)
   {
      refCurves = 0;
   }
   else if (refSrc == RS_frame)
   {
      refCurves = &node;
   }
   else
   {
      AlembicScene *refScene = mDso->referenceScene();
      
      if (!refScene)
      {
         refCurves = 0;
      }
      else
      {
         AlembicNode *refNode = refScene->find(node.path());
         
         if (refNode)
         {
            refCurves = dynamic_cast<AlembicCurves*>(refNode);
         }
         else
         {
            refCurves = 0;
         }
      }
   }
   
   return hasPref;
}

bool MakeShape::initCurves(CurvesInfo &info,
                           const Alembic::AbcGeom::ICurvesSchema::Sample &sample,
                           std::string &arnoldBasis)
{
   if (sample.getType() == Alembic::AbcGeom::kVariableOrder)
   {
      AiMsgWarning("[abcproc] Variable order curves not supported");
      return false;
   }
   
   bool nurbs = false;
   int degree = (sample.getType() == Alembic::AbcGeom::kCubic ? 3 : 1);
   bool periodic = (sample.getWrap() == Alembic::AbcGeom::kPeriodic);
   
   if (info.degree != 1)
   {
      switch (sample.getBasis())
      {
      case Alembic::AbcGeom::kNoBasis:
         arnoldBasis = "linear";
         degree = 1;
         break;
      
      case Alembic::AbcGeom::kBezierBasis:
         arnoldBasis = "bezier";
         break;
      
      case Alembic::AbcGeom::kBsplineBasis:
         nurbs = true;
         arnoldBasis = "catmull-rom";
         break;
      
      case Alembic::AbcGeom::kCatmullromBasis:
         arnoldBasis = "catmull-rom";
         break;
      
      case Alembic::AbcGeom::kPowerBasis:
         AiMsgWarning("[abcproc] Curve 'Power' basis not supported");
         return false;
      
      case Alembic::AbcGeom::kHermiteBasis:
         AiMsgWarning("[abcproc] Curve 'Hermite' basis not supported");
         return false;
      
      default:
         AiMsgWarning("[abcproc] Unknown curve basis");
         return false;
      }
   }
   
   if (info.degree != -1)
   {
      if (degree != info.degree ||
          nurbs != info.nurbs ||
          periodic != info.periodic)
      {
         return false;
      }
   }
   
   info.degree = degree;
   info.periodic = periodic;
   info.nurbs = nurbs;
   
   return true;
}

bool MakeShape::fillCurvesPositions(CurvesInfo &info,
                                    size_t motionStep,
                                    size_t numMotionSteps,
                                    Alembic::Abc::Int32ArraySamplePtr Nv,
                                    Alembic::Abc::P3fArraySamplePtr P0,
                                    Alembic::Abc::FloatArraySamplePtr W0,
                                    Alembic::Abc::P3fArraySamplePtr P1,
                                    Alembic::Abc::FloatArraySamplePtr W1,
                                    const float *vel,
                                    const float *acc,
                                    float blend,
                                    Alembic::Abc::FloatArraySamplePtr K,
                                    AtArray* &num_points,
                                    AtArray* &points)
{
   unsigned int extraPoints = (info.degree == 3 ? 2 : 0);
   
   if (!num_points)
   {
      info.curveCount = (unsigned int) Nv->size();
      info.pointCount = 0;
      info.cvCount = 0;
      
      num_points = AiArrayAllocate(info.curveCount, 1, AI_TYPE_UINT);
      
      if (info.nurbs)
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            // Num Spans + Degree = Num CVs
            // -> Num Spans = Num CVs - degree
            unsigned int curvePointCount = 1 + (Nv->get()[ci] - info.degree) * mDso->nurbsSampleRate();
            
            AiArraySetUInt(num_points, ci, curvePointCount + extraPoints);
            
            info.pointCount += curvePointCount;
         }
         
         info.cvCount = (unsigned int) P0->size();
      }
      else
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            AiArraySetUInt(num_points, ci, (unsigned int) (Nv->get()[ci] + extraPoints));
         }
         
         info.pointCount = (unsigned int) P0->size();
      }
   }
   else if (AiArrayGetNumElements(num_points) != info.curveCount)
   {
      AiMsgWarning("[abcproc] Bad curve num_points array size");
      return false;
   }
   
   if (info.pointCount == 0)
   {
      if (info.nurbs)
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            info.pointCount += (1 + (Nv->get()[ci] - info.degree) * mDso->nurbsSampleRate());
         }
      }
      else
      {
         info.pointCount = (unsigned int) P0->size();
      }
   }
   
   unsigned int allocPointCount = info.pointCount + (extraPoints * info.curveCount);
   
   if (!points)
   {
      points = AiArrayAllocate(allocPointCount, numMotionSteps, AI_TYPE_VECTOR);
   }
   else
   {
      if (AiArrayGetNumElements(points) != allocPointCount)
      {
         AiMsgWarning("[abcproc] Bad curve points array size");
         return false;
      }
      else if (motionStep >= AiArrayGetNumKeys(points))
      {
         AiMsgWarning("[abcproc] Bad motion key for curve points");
         return false;
      }
   }
   
   size_t pi = motionStep * allocPointCount;
   unsigned int po = 0;
   Alembic::Abc::V3f p0, p1;
   AtVector pnt;
   const float *cvel = vel;
   const float *cacc = acc;
   
   if (info.nurbs)
   {
      NURBS<3> curve;
      NURBS<3>::CV cv;
      NURBS<3>::Pnt pt;
      unsigned int ko = 0;
      float w0, w1;
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Allocate %u point(s) for %u curve(s), from %lu cv(s)", info.pointCount, info.curveCount, P0->size());
      }
      
      for (unsigned int ci=0; ci<info.curveCount; ++ci)
      {
         // Build NURBS curve
         
         long np = Nv->get()[ci];
         long ns = np - info.degree;
         long nk = ns + 2 * info.degree + 1;
         
         curve.setNumCVs(np);
         curve.setNumKnots(nk);
         
         if (blend > 0.0f && (P1 || vel))
         {
            if (P1)
            {
               float a = 1.0f - blend;
               
               for (long i=0; i<np; ++i)
               {
                  p0 = P0->get()[po + i];
                  p1 = P1->get()[po + i];
                  
                  w0 = (W0 ? W0->get()[po + i] : 1.0f);
                  w1 = (W1 ? W1->get()[po + i] : 1.0f);
                  
                  cv[0] = a * p0.x + blend * p1.x;
                  cv[1] = a * p0.y + blend * p1.y;
                  cv[2] = a * p0.z + blend * p1.z;
                  cv[3] = a * w0 + blend * w1;
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
                  // }
                  
                  curve.setCV(i, cv);
               }
            }
            else if (acc)
            {
               for (long i=0; i<np; ++i, cvel+=3, cacc+=3)
               {
                  p0 = P0->get()[po + i];
                  w0 = (W0 ? W0->get()[po + i] : 1.0f);
                  
                  cv[0] = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  cv[1] = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  cv[2] = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  cv[3] = w0;
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
                  // }
                  
                  curve.setCV(i, cv);
               }
            }
            else
            {
               for (long i=0; i<np; ++i, cvel+=3)
               {
                  p0 = P0->get()[po + i];
                  w0 = (W0 ? W0->get()[po + i] : 1.0f);
                  
                  cv[0] = p0.x + blend * cvel[0];
                  cv[1] = p0.y + blend * cvel[1];
                  cv[2] = p0.z + blend * cvel[2];
                  cv[3] = w0;
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
                  // }
                  
                  curve.setCV(i, cv);
               }
            }
         }
         else
         {
            for (long i=0; i<np; ++i)
            {
               p0 = P0->get()[po + i];
               w0 = (W0 ? W0->get()[po + i] : 1.0f);
               
               cv[0] = p0.x;
               cv[1] = p0.y;
               cv[2] = p0.z;
               cv[3] = w0;
               
               // if (mDso->verbose())
               // {
               //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
               // }
               
               curve.setCV(i, cv);
            }
         }
         
         for (long i=0; i<nk; ++i)
         {
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] curve[%u].knot[%ld] = %f", ci, i, K->get()[ko + i]);
            // }
            
            curve.setKnot(i, K->get()[ko + i]);
         }
         
         po += np;
         ko += nk;
         
         // Evaluate NURBS
         
         float ustart = 0.0f;
         float uend = 0.0f;
         
         curve.getDomain(ustart, uend);
         
         float ustep = (uend - ustart) / float(ns * mDso->nurbsSampleRate());
         
         long numSamples = 1 + ns * mDso->nurbsSampleRate();
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] curve[%u] domain = [%f, %f], step = %f, points = %ld", ci, ustart, uend, ustep, numSamples);
         }
         
         if (extraPoints > 0)
         {
            curve.eval(ustart, pt);
            pnt.x = pt[0];
            pnt.y = pt[1];
            pnt.z = pt[2];
            AiArraySetVec(points, pi++, pnt);
            
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] first point (u=%f, idx=%lu): (%f, %f, %f)", ustart, pi-1, pnt.x, pnt.y, pnt.z);
            // }
         }
         
         float u = ustart;
         
         for (long i=0; i<numSamples; ++i)
         {
            curve.eval(u, pt);
            pnt.x = pt[0];
            pnt.y = pt[1];
            pnt.z = pt[2];
            AiArraySetVec(points, pi++, pnt);
            
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] point(u=%f, idx=%lu): (%f, %f, %f)", u, pi-1, pnt.x, pnt.y, pnt.z);
            // }
         
            u += ustep;
         }
         
         if (extraPoints > 0)
         {
            curve.eval(uend, pt);
            pnt.x = pt[0];
            pnt.y = pt[1];
            pnt.z = pt[2];
            AiArraySetVec(points, pi++, pnt);
            
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] last point(u=%f, idx=%lu): (%f, %f, %f)", uend, pi-1, pnt.x, pnt.y, pnt.z);
            // }
         }
      }
   }
   else
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] %u curve(s), %u point(s)", info.curveCount, info.pointCount);
      }
      
      if (blend > 0.0f && (P1 || vel))
      {
         if (P1)
         {
            float a = 1.0f - blend;
            
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               int np = Nv->get()[ci];
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po];
                  p1 = P1->get()[po]; 
                  pnt.x = a * p0.x + blend * p1.x;
                  pnt.y = a * p0.y + blend * p1.y;
                  pnt.z = a * p0.z + blend * p1.z;
                  AiArraySetVec(points, pi++, pnt);
               }
               
               for (int i=0; i<np; ++i)
               {
                  p0 = P0->get()[po + i];
                  p1 = P1->get()[po + i];
                  pnt.x = a * p0.x + blend * p1.x;
                  pnt.y = a * p0.y + blend * p1.y;
                  pnt.z = a * p0.z + blend * p1.z;
                  AiArraySetVec(points, pi++, pnt);
               }
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po + np - 1];
                  p1 = P1->get()[po + np - 1];
                  pnt.x = a * p0.x + blend * p1.x;
                  pnt.y = a * p0.y + blend * p1.y;
                  pnt.z = a * p0.z + blend * p1.z;
                  AiArraySetVec(points, pi++, pnt);
               }
               
               po += np;
            }
         }
         else if (acc)
         {
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               int np = Nv->get()[ci];
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po];
                  pnt.x = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  pnt.y = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  pnt.z = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  AiArraySetVec(points, pi++, pnt);
               }
               
               for (int i=0; i<np; ++i, cvel+=3, cacc+=3)
               {
                  p0 = P0->get()[po + i];
                  pnt.x = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  pnt.y = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  pnt.z = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  AiArraySetVec(points, pi++, pnt);
               }
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po + np - 1];
                  pnt.x = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  pnt.y = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  pnt.z = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  AiArraySetVec(points, pi++, pnt);
               }
               
               po += np;
            }
         }
         else
         {
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               int np = Nv->get()[ci];
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po];
                  pnt.x = p0.x + blend * cvel[0];
                  pnt.y = p0.y + blend * cvel[1];
                  pnt.z = p0.z + blend * cvel[2];
                  AiArraySetVec(points, pi++, pnt);
               }
               
               for (int i=0; i<np; ++i, cvel+=3)
               {
                  p0 = P0->get()[po + i];
                  pnt.x = p0.x + blend * cvel[0];
                  pnt.y = p0.y + blend * cvel[1];
                  pnt.z = p0.z + blend * cvel[2];
                  AiArraySetVec(points, pi++, pnt);
               }
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po + np - 1];
                  pnt.x = p0.x + blend * cvel[0];
                  pnt.y = p0.y + blend * cvel[1];
                  pnt.z = p0.z + blend * cvel[2];
                  AiArraySetVec(points, pi++, pnt);
               }
               
               po += np;
            }
         }
      }
      else
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            int np = Nv->get()[ci];
            
            if (extraPoints > 0)
            {
               p0 = P0->get()[po];
               pnt.x = p0.x;
               pnt.y = p0.y;
               pnt.z = p0.z;
               AiArraySetVec(points, pi++, pnt);
            }
            
            for (int i=0; i<np; ++i, cvel+=3, cacc+=3)
            {
               p0 = P0->get()[po + i];
               pnt.x = p0.x;
               pnt.y = p0.y;
               pnt.z = p0.z;
               AiArraySetVec(points, pi++, pnt);
            }
            
            if (extraPoints > 0)
            {
               p0 = P0->get()[po + np - 1];
               pnt.x = p0.x;
               pnt.y = p0.y;
               pnt.z = p0.z;
               AiArraySetVec(points, pi++, pnt);
            }
            
            po += np;
         }
      }
   }
   
   return true;
}

bool MakeShape::fillReferencePositions(AlembicCurves *refCurves,
                                       CurvesInfo &info,
                                       Alembic::Abc::Int32ArraySamplePtr Nv,
                                       Alembic::Abc::FloatArraySamplePtr W,
                                       Alembic::Abc::FloatArraySamplePtr K)
{
   const std::string &PrefName = mDso->referencePositionName();
      
   bool hasPref = false;
   
   if (!refCurves)
   {
      UserAttributes::iterator uait = info.pointAttrs.find(PrefName);
      hasPref = (uait != info.pointAttrs.end());
      
      if (!hasPref)
      {
         return false;
      }
      
      UserAttribute &ua = uait->second;
      
      unsigned int count = (info.nurbs ? info.cvCount : info.pointCount);
      
      // Little type aliasing: float[1][3n] -> float[3][n]
      if (ua.dataDim == 1 && ua.dataCount == (3 * count))
      {
         AiMsgWarning("[abcproc] \"%s\" exported with wrong base type: float instead if float[3]", PrefName.c_str());
         
         ua.dataDim = 3;
         ua.dataCount = count;
         ua.arnoldType = AI_TYPE_VECTOR;
         ua.arnoldTypeStr = "VECTOR";
      }
      
      // Check valid point count
      if (ua.dataCount != count || ua.dataDim != 3)
      {
         AiMsgWarning("[abcproc] \"%s\" exported with incompatible size and/or dimension", PrefName.c_str());
         
         DestroyUserAttribute(ua);
         info.pointAttrs.erase(info.pointAttrs.find(PrefName));
         return false;
      }
      
      if (info.nurbs)
      {
         if (!Nv || Nv->size() != info.curveCount)
         {
            DestroyUserAttribute(ua);
            info.pointAttrs.erase(info.pointAttrs.find(PrefName));
            return false;
         }
         
         if (!W || W->size() != count)
         {
            DestroyUserAttribute(ua);
            info.pointAttrs.erase(info.pointAttrs.find(PrefName));
            return false;
         }
         
         NURBS<3> curve;
         NURBS<3>::CV cv;
         NURBS<3>::Pnt pnt;
         Alembic::Abc::V3f p;
         const float *P = (const float*) ua.data;
         float *newP = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
         long ko = 0;
         long po = 0;
         size_t off = 0;
         
         for (size_t ci=0, pi=0; ci<Nv->size(); ++ci)
         {
            // Build NURBS curve
            
            long np = Nv->get()[ci];
            long ns = np - info.degree;
            long nk = ns + 2 * info.degree + 1;
            
            curve.setNumCVs(np);
            curve.setNumKnots(nk);
            
            for (int pi=0; pi<np; ++pi, P+=3)
            {
               cv[0] = P[0];
               cv[1] = P[1];
               cv[2] = P[2];
               cv[3] = (W ? W->get()[po + pi] : 1.0f);
               curve.setCV(pi, cv);
            }
            
            for (int ki=0; ki<nk; ++ki)
            {
               curve.setKnot(ki, K->get()[ko + ki]);
            }
            
            po += np;
            ko += nk;
            
            // Evaluate NURBS curve
            
            float ustart = 0.0f;
            float uend = 0.0f;
            
            curve.getDomain(ustart, uend);
            
            float ustep = (uend - ustart) / (ns * mDso->nurbsSampleRate());
            
            long n = 1 + (ns * mDso->nurbsSampleRate());
            
            float u = ustart;
            
            for (int i=0; i<n; ++i, off+=3)
            {
               curve.eval(u, pnt);
               
               // check off < 3 * info.pointCount?
               newP[off + 0] = pnt[0];
               newP[off + 1] = pnt[1];
               newP[off + 2] = pnt[2];
               
               u += ustep;
            }
         }
         
         AiFree(ua.data);
         ua.data = newP;
         ua.dataDim = 3;
         ua.dataCount = info.pointCount;
      }
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Read reference positions from user attribute \"%s\".", PrefName.c_str());
      }
   }
   else
   {
      TimeSample<Alembic::AbcGeom::ICurvesSchema> sampler;
      Alembic::AbcGeom::ICurvesSchema schema = refCurves->typedObject().getSchema();
      Alembic::AbcGeom::ICurvesSchema::Sample sample;
      
      size_t n = schema.getNumSamples();
      
      if (n == 0)
      {
         return false;
      }
      
      double refTime = schema.getTimeSampling()->getSampleTime(0);
      
      if (mDso->referenceSource() == RS_frame)
      {
         double minTime = refTime;
         double maxTime = schema.getTimeSampling()->getSampleTime(n-1);
         
         refTime = mDso->referenceFrame() / mDso->fps();
         if (refTime < minTime) refTime = minTime;
         if (refTime > maxTime) refTime = maxTime;
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Read reference positions from user frame %f.", mDso->referenceFrame());
         }
      }
      else
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Read reference positions from separate file.");
         }
      }
      
      if (!sampler.get(schema, refTime))
      {
         return false;
      }
      
      sample = sampler.data();
      
      Alembic::Abc::P3fArraySamplePtr Pref = sample.getPositions();
      Alembic::Abc::Int32ArraySamplePtr Nv = sample.getCurvesNumVertices();
      
      if (Nv->size() == info.curveCount &&
          Pref->size() == (info.nurbs ? info.cvCount : info.pointCount))
      {
         // Figure out world matrix
         
         Alembic::Abc::M44d Mref;
         
         // recompute matrix
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Compute reference world matrix");
         }
      
         AlembicXform *refParent = dynamic_cast<AlembicXform*>(refCurves->parent());
      
         while (refParent)
         {
            Alembic::AbcGeom::IXformSchema xformSchema = refParent->typedObject().getSchema();
         
            Alembic::AbcGeom::XformSample xformSample = xformSchema.getValue();
         
            Mref = Mref * xformSample.getMatrix();
         
            if (xformSchema.getInheritsXforms())
            {
               refParent = dynamic_cast<AlembicXform*>(refParent->parent());
            }
            else
            {
               refParent = 0;
            }
         }
         
         // Compute reference positions
         
         float *vals = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
         
         if (info.nurbs)
         {
            Alembic::Abc::FloatArraySamplePtr W = sample.getPositionWeights();
            Alembic::Abc::FloatArraySamplePtr K;
            
            schema.getKnotsProperty().get(K);
            
            NURBS<3> curve;
            NURBS<3>::CV cv;
            NURBS<3>::Pnt pnt;
            Alembic::Abc::V3f p;
            long ko = 0;
            long po = 0;
            
            for (size_t ci=0, off=0; ci<Nv->size(); ++ci)
            {
               // Build NURBS curve
               
               long np = Nv->get()[ci];
               long ns = np - info.degree;
               long nk = ns + 2 * info.degree + 1;
               
               curve.setNumCVs(np);
               curve.setNumKnots(nk);
               
               for (int pi=0; pi<np; ++pi)
               {
                  p = Pref->get()[po + pi] * Mref;
                  cv[0] = p.x;
                  cv[1] = p.y;
                  cv[2] = p.z;
                  cv[3] = (W ? W->get()[po + pi] : 1.0f);
                  curve.setCV(pi, cv);
               }
               
               for (int ki=0; ki<nk; ++ki)
               {
                  curve.setKnot(ki, K->get()[ko + ki]);
               }
               
               po += np;
               ko += nk;
               
               // Evaluate curve
               
               float ustart = 0.0f;
               float uend = 0.0f;
               
               curve.getDomain(ustart, uend);
               
               float ustep = (uend - ustart) / (ns * mDso->nurbsSampleRate());
               
               long n = 1 + (ns * mDso->nurbsSampleRate());
               
               float u = ustart;
               
               for (int i=0; i<n; ++i, off+=3)
               {
                  curve.eval(u, pnt);
                  
                  vals[off + 0] = pnt[0];
                  vals[off + 1] = pnt[1];
                  vals[off + 2] = pnt[2];
                  
                  u += ustep;
               }
            }
         }
         else
         {
            for (unsigned int i=0, off=0; i<info.pointCount; ++i, off+=3)
            {
               Alembic::Abc::V3f p = Pref->get()[i] * Mref;
               
               vals[off + 0] = p.x;
               vals[off + 1] = p.y;
               vals[off + 2] = p.z;
            }
         }
         
         UserAttribute &ua = info.pointAttrs[PrefName];
         
         InitUserAttribute(ua);
         
         ua.arnoldCategory = AI_USERDEF_VARYING;
         ua.arnoldType = AI_TYPE_VECTOR;
         ua.arnoldTypeStr = "VECTOR";
         ua.isArray = true;
         ua.dataDim = 3;
         ua.dataCount = info.pointCount;
         ua.data = vals;
         
         hasPref = true;
      }
      else
      {
         AiMsgWarning("[abcproc] Could not generate \"%s\" for curves \"%s\"", PrefName.c_str(), refCurves->path().c_str());
      }
   }
   
   return hasPref;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicCurves &node, AlembicNode *instance)
{
   Alembic::AbcGeom::ICurvesSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(node, instance);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   CurvesInfo info;
   
   info.varyingTopology = (mDso->forceVelocityBlur() || schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology);
   
   TimeSampleList<Alembic::AbcGeom::ICurvesSchema> &samples = node.samples().schemaSamples;
   TimeSampleList<Alembic::AbcGeom::IFloatGeomParam> Wsamples;
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> Nsamples;
   TimeSampleList<Alembic::AbcGeom::ICurvesSchema>::ConstIterator samp0, samp1;
   TimeSampleList<Alembic::AbcGeom::IFloatGeomParam>::ConstIterator Wsamp0, Wsamp1;
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam>::ConstIterator Nsamp0, Nsamp1;
   double a = 1.0;
   double b = 0.0;
   double t = 0.0;
   
   Alembic::AbcGeom::IFloatGeomParam widths = schema.getWidthsParam();
   Alembic::AbcGeom::IN3fGeomParam normals = schema.getNormalsParam();
   Alembic::AbcGeom::IV2fGeomParam uvs = schema.getUVsParam();
   
   if (info.varyingTopology)
   {
      t = mDso->renderTime();
      
      node.sampleSchema(t, t, false);
      
      if (widths)
      {
         Wsamples.update(widths, t, t, false);
      }
      
      if (normals)
      {
         Nsamples.update(normals, t, t, false);
      }
   }
   else
   {
      for (size_t i=0; i<mDso->numMotionSamples(); ++i)
      {
         t = mDso->motionSampleTime(i);
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Sample curves \"%s\" at t=%lf", node.path().c_str(), t);
         }
      
         node.sampleSchema(t, t, i > 0);
         
         if (widths)
         {
            Wsamples.update(widths, t, t, i > 0);
         }
         
         if (normals)
         {
            Nsamples.update(normals, t, t, i > 0);
         }
      }
      
      // renderTime() may not be in the sample list, let's at it just in case
      node.sampleSchema(mDso->renderTime(), mDso->renderTime(), true);
   }
   
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Read %lu curves samples", samples.size());
   }
   
   if (samples.size() == 0)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Collect attributes
   
   double attribsTime = (info.varyingTopology ? mDso->renderTime() : mDso->attributesTime(mDso->attributesEvaluationTime()));
   
   bool interpolateAttribs = !info.varyingTopology;
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         attribsTime,
                         interpolateAttribs,
                         &info.objectAttrs,
                         &info.primitiveAttrs,
                         &info.pointAttrs,
                         0,
                         0);
   
   // Process point positions
   
   AtArray *num_points = 0;
   AtArray *points = 0;
   std::string basis = "linear";
   Alembic::Abc::Int32ArraySamplePtr Nv;
   Alembic::Abc::P3fArraySamplePtr P0;
   Alembic::Abc::P3fArraySamplePtr P1;
   Alembic::Abc::FloatArraySamplePtr W0;
   Alembic::Abc::FloatArraySamplePtr W1;
   Alembic::Abc::FloatArraySamplePtr K;
   bool success = false;
   
   if (samples.size() == 1 && !info.varyingTopology)
   {
      samp0 = samples.begin();
      
      Nv = samp0->data().getCurvesNumVertices();
      P0 = samp0->data().getPositions();
      
      if (initCurves(info, samp0->data(), basis))
      {
         if (info.nurbs)
         {
            W0 = samp0->data().getPositionWeights();
            
            // getKnots() is not 'const'! WTF!
            //K = samp0->data().getKnots();
            
            Alembic::Abc::ISampleSelector iss(samp0->time(), Alembic::Abc::ISampleSelector::kNearIndex);
            schema.getKnotsProperty().get(K, iss);
         }
         
         success = fillCurvesPositions(info, 0, 1, Nv, P0, W0, P1, W1, 0, 0, 0.0f, K, num_points, points);
      }
   }
   else
   {
      if (info.varyingTopology)
      {
         samples.getSamples(mDso->renderTime(), samp0, samp1, b);
         
         Nv = samp0->data().getCurvesNumVertices();
         P0 = samp0->data().getPositions();
         
         if (initCurves(info, samp0->data(), basis))
         {
            if (info.nurbs)
            {
               W0 = samp0->data().getPositionWeights();
               
               Alembic::Abc::ISampleSelector iss(samp0->time(), Alembic::Abc::ISampleSelector::kNearIndex);
               schema.getKnotsProperty().get(K, iss);
            }
            
            // Get velocity
            
            const float *vel = 0;
            const char *velName = mDso->velocityName();
            UserAttributes::iterator it = info.pointAttrs.end();
            
            if (velName)
            {
               if (strcmp(velName, "<builtin>") != 0)
               {
                  it = info.pointAttrs.find(velName);
                  if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
                  {
                     it = info.pointAttrs.end();
                  }
               }
            }
            else
            {
               it = info.pointAttrs.find("velocity");
               if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
               {
                  it = info.pointAttrs.find("vel");
                  if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
                  {
                     it = info.pointAttrs.find("v");
                     if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
                     {
                        it = info.pointAttrs.end();
                     }
                  }
               }
            }
            
            if (it != info.pointAttrs.end())
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Using user defined attribute \"%s\" for point velocities", it->first.c_str());
               }
               vel = (const float*) it->second.data;
            }
            else if (samp0->data().getVelocities())
            {
               Alembic::Abc::V3fArraySamplePtr V0 = samp0->data().getVelocities();
               if (V0->size() != P0->size())
               {
                  AiMsgWarning("[abcproc] Velocities count doesn't match points' one (%lu for %lu). Ignoring it.", V0->size(), P0->size());
               }
               else
               {
                  vel = (const float*) V0->getData();
               }
            }
            
            // Get acceleration
            
            const float *acc = 0;
            
            if (vel)
            {
               const char *accName = mDso->accelerationName();
               UserAttributes::iterator it = info.pointAttrs.end();
               
               if (accName)
               {
                  it = info.pointAttrs.find(accName);
                  if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
                  {
                     it = info.pointAttrs.end();
                  }
               }
               else
               {
                  it = info.pointAttrs.find("acceleration");
                  if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
                  {
                     it = info.pointAttrs.find("accel");
                     if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
                     {
                        it = info.pointAttrs.find("a");
                        if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
                        {
                           it = info.pointAttrs.end();
                        }
                     }
                  }
               }
               
               if (it != info.pointAttrs.end())
               {
                  acc = (const float*) it->second.data;
               }
            }
            
            // Compute positions
            
            if (vel)
            {
               success = true;
               
               for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
               {
                  //t = mDso->motionSampleTime(i) - mDso->renderTime();
                  t = mDso->motionSampleTime(i) - samp0->time();
                  t *= mDso->velocityScale();
                  
                  if (!fillCurvesPositions(info, i, mDso->numMotionSamples(), Nv, P0, W0, P1, W1, vel, acc, t, K, num_points, points))
                  {
                     success = false;
                     break;
                  }
               }
            }
            else
            {
               // only one sample
               success = fillCurvesPositions(info, 0, 1, Nv, P0, W0, P1, W1, 0, 0, 0.0f, K, num_points, points);
            }
         }
      }
      else
      {
         success = true;
         
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            b = 0.0;
            t = mDso->motionSampleTime(i);
            
            samples.getSamples(t, samp0, samp1, b);
            
            if (!initCurves(info, samp0->data(), basis))
            {
               success = false;
               break;
            }
            
            Nv = samp0->data().getCurvesNumVertices();
            P0 = samp0->data().getPositions();
            
            if (info.nurbs)
            {
               W0 = samp0->data().getPositionWeights();
               
               Alembic::Abc::ISampleSelector iss(samp0->time(), Alembic::Abc::ISampleSelector::kNearIndex);
               schema.getKnotsProperty().get(K, iss);
            }
            
            if (b > 0.0)
            {
               if (!initCurves(info, samp1->data(), basis))
               {
                  success = false;
                  break;
               }
               
               P1 = samp1->data().getPositions();
               
               if (info.nurbs)
               {
                  W1 = samp0->data().getPositionWeights();
               }
            }
            else
            {
               P1.reset();
               W1.reset();
            }
            
            if (!fillCurvesPositions(info, i, mDso->numMotionSamples(), Nv, P0, W0, P1, W1, 0, 0, b, K, num_points, points))
            {
               success = false;
               break;
            }
         }
      }
   }
   
   if (!success)
   {
      if (num_points)
      {
         AiArrayDestroy(num_points);
      }
      
      if (points)
      {
         AiArrayDestroy(points);
      }
      
      return AlembicNode::DontVisitChildren;
   }
   
   mNode = createArnoldNode(Strings::curves, (instance ? *instance : node), true);
   AiNodeSetStr(mNode, Strings::basis, basis.c_str());
   AiNodeSetArray(mNode, Strings::num_points, num_points);
   AiNodeSetArray(mNode, Strings::points, points);
   AiNodeSetStr(mNode, Strings::mode, "ribbon");
   // mode
   // min_pixel_width
   // invert_normals
   
   // Process radius
   AtArray *radius = 0;
   
   if (Wsamples.size() > 0)
   {
      Alembic::Abc::FloatArraySamplePtr W0, W1;
      
      if (Wsamples.size() > 1 && !info.varyingTopology)
      {
         radius = AiArrayAllocate(info.pointCount, mDso->numMotionSamples(), AI_TYPE_FLOAT);
         
         unsigned int po = 0;
         
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            b = 0.0;
            
            Wsamples.getSamples(mDso->motionSampleTime(i), Wsamp0, Wsamp1, b);
            
            W0 = Wsamp0->data().getVals();
            
            if (W0->size() != 1 &&
                W0->size() != info.curveCount &&
                W0->size() != info.pointCount) // W0->size() won't match pointCount for NURBS
            {
               AiArrayDestroy(radius);
               radius = 0;
               break;
            }
            
            unsigned int Wcount = (unsigned int) W0->size();
            unsigned int pi = 0;
            
            if (b > 0.0)
            {
               a = 1.0 - b;
               
               W1 = Wsamp1->data().getVals();
               
               if (W1->size() != W0->size())
               {
                  AiArrayDestroy(radius);
                  radius = 0;
                  break;
               }
               
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int wi = (Wcount == 1 ? 0 : (Wcount == info.curveCount ? ci : pi));
                     
                     float w0 = W0->get()[wi];
                     float w1 = W1->get()[wi];
                     
                     AiArraySetFlt(radius, po + pi, 0.5f * adjustWidth((a * w0 + b * w1)));
                  }
               }
            }
            else
            {
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int wi = (Wcount == 1 ? 0 : (Wcount == info.curveCount ? ci : pi));
                     
                     AiArraySetFlt(radius, po + pi, 0.5f * adjustWidth(W0->get()[wi]));
                  }
               }
            }
            
            po += info.pointCount;
         }
      }
      else
      {
         Wsamples.getSamples(mDso->renderTime(), Wsamp0, Wsamp1, b);
            
         W0 = Wsamp0->data().getVals();
         
         if (W0->size() == 1 ||
             W0->size() == info.curveCount ||
             W0->size() == info.pointCount)
         {
            unsigned int Wcount = (unsigned int) W0->size();
         
            radius = AiArrayAllocate(info.pointCount, 1, AI_TYPE_FLOAT);
            
            unsigned int pi = 0;
            
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
               
               for (unsigned int j=0; j<np; ++j, ++pi)
               {
                  unsigned int wi = (Wcount == 1 ? 0 : (Wcount == info.curveCount ? ci : pi));
                  
                  AiArraySetFlt(radius, pi, 0.5f * adjustWidth(W0->get()[wi]));
               }
            }
         }
      }
   }
   
   if (!radius)
   {
      AiMsgWarning("[abcproc] Defaulting curve width to minimum width (use -widthmin parameter to override)");
      
      radius = AiArrayAllocate(info.pointCount, 1, AI_TYPE_FLOAT);
      
      for (unsigned int i=0; i<info.pointCount; ++i)
      {
         AiArraySetFlt(radius, i, 0.5f * mDso->widthMin());
      }
   }
   
   AiNodeSetArray(mNode, "radius", radius);
   
   // Process normals (orientation)
   
   AtArray *orientations = 0;
   
   if (Nsamples.size() > 0)
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Reading curve normal(s)...");
      }
      
      Alembic::Abc::N3fArraySamplePtr N0, N1;
      Alembic::Abc::V3f n0, n1;
      AtVector nrm;
         
      if (Nsamples.size() > 1 && !info.varyingTopology)
      {
         orientations = AiArrayAllocate(info.pointCount, mDso->numMotionSamples(), AI_TYPE_VECTOR);
         
         unsigned int po = 0;
         
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            b = 0.0;
            
            Nsamples.getSamples(mDso->motionSampleTime(i), Nsamp0, Nsamp1, b);
            
            N0 = Nsamp0->data().getVals();
            
            if (N0->size() != 1 &&
                N0->size() != info.curveCount &&
                N0->size() != info.pointCount)
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Curve normals count mismatch");
               }
               
               AiArrayDestroy(orientations);
               orientations = 0;
               
               break;
            }
            
            unsigned int Ncount = (unsigned int) N0->size();
            unsigned int pi = 0;
            
            if (b > 0.0)
            {
               a = 1.0 - b;
               
               N1 = Nsamp1->data().getVals();
               
               if (N1->size() != N0->size())
               {
                  AiArrayDestroy(orientations);
                  orientations = 0;
                  break;
               }
               
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int ni = (Ncount == 1 ? 0 : (Ncount == info.curveCount ? ci : pi));
                     
                     n0 = N0->get()[ni];
                     n1 = N1->get()[ni];
                     nrm.x = a * n0.x + b * n1.x;
                     nrm.y = a * n0.y + b * n1.y;
                     nrm.z = a * n0.z + b * n1.z;
                     AiArraySetVec(orientations, po + pi, nrm);
                  }
               }
            }
            else
            {
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int ni = (Ncount == 1 ? 0 : (Ncount == info.curveCount ? ci : pi));
                     
                     n0 = N0->get()[ni];
                     nrm.x = n0.x;
                     nrm.y = n0.y;
                     nrm.z = n0.z;
                     AiArraySetVec(orientations, po + pi, nrm);
                  }
               }
            }
            
            po += info.pointCount;
         }
      }
      else
      {
         Nsamples.getSamples(mDso->renderTime(), Nsamp0, Nsamp0, b);
            
         N0 = Nsamp0->data().getVals();
         
         if (N0->size() == 1 ||
             N0->size() == info.curveCount ||
             N0->size() == info.pointCount)
         {
            unsigned int Ncount = (unsigned int) N0->size();
         
            orientations = AiArrayAllocate(info.pointCount, 1, AI_TYPE_VECTOR);
            
            unsigned int pi = 0;
            
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
               
               for (unsigned int j=0; j<np; ++j, ++pi)
               {
                  unsigned int ni = (Ncount == 1 ? 0 : (Ncount == info.curveCount ? ci : pi));
                  
                  n0 = N0->get()[ni];
                  nrm.x = n0.x;
                  nrm.y = n0.y;
                  nrm.z = n0.z;
                  AiArraySetVec(orientations, pi, nrm);
               }
            }
         }
         else if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Curve normals count mismatch");
         }
      }
   }
   
   if (orientations)
   {
      AiNodeSetStr(mNode, "mode", "oriented");
      AiNodeSetArray(mNode, "orientations", orientations);
   }
   
   // Process uvs
   
   if (uvs)
   {
      if (info.pointAttrs.find("uv") != info.pointAttrs.end() ||
          info.primitiveAttrs.find("uv") != info.primitiveAttrs.end() ||
          info.objectAttrs.find("uv") != info.objectAttrs.end())
      {
         AiMsgWarning("[abcproc] Curves node already has a 'uv' user attribute, ignore alembic's");
      }
      else
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Reading curve uv(s)...");
         }
         
         Alembic::Abc::ICompoundProperty uvsProp = uvs.getValueProperty().getParent();
         
         UserAttribute uvsAttr;
         
         InitUserAttribute(uvsAttr);
         
         if (ReadUserAttribute(uvsAttr, uvsProp.getParent(), uvsProp.getHeader(), attribsTime, true, interpolateAttribs))
         {
            switch (uvsAttr.arnoldCategory)
            {
            case AI_USERDEF_CONSTANT:
               SetUserAttribute(mNode, "uv", uvsAttr, 0);
               break;
            case AI_USERDEF_UNIFORM:
               SetUserAttribute(mNode, "uv", uvsAttr, info.curveCount);
               break;
            case AI_USERDEF_VARYING:
               SetUserAttribute(mNode, "uv", uvsAttr, info.pointCount);
               break;
            }
         }
         
         DestroyUserAttribute(uvsAttr);
      }
   }
   
   // Reference positions
   
   if (mDso->outputReference())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate reference attributes");
      }
      
      AlembicCurves *refCurves = 0;
      
      bool hasPref = getReferenceCurves(node, info, refCurves);
      
      if (hasPref || refCurves)
      {
         hasPref = fillReferencePositions(refCurves, info, Nv, W0, K);
      }
      
      if (!hasPref)
      {
         AiMsgWarning("[abcproc] Invalid reference object specification (%s).", mDso->objectPath().c_str());
      }
   }
   
   removeConflictingAttribs(mNode, &(info.objectAttrs), &(info.primitiveAttrs), &(info.pointAttrs), 0, mDso->verbose());
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.primitiveAttrs, info.curveCount);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
      
   outputInstanceNumber(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicNode &, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
}

void MakeShape::leave(AlembicNode &, AlembicNode *)
{
}
