#include "_precl.h"


const UInt32 COP_WITH_STINGER_CHANCE = 32;
const char STINGER_MODEL_NAME[] = "plc_stinger";
const UInt32 MAX_STINGER_SEGMENTS = 12;
const UInt32 MAX_STINGERS = 2;
const UInt32 DEPLOYTIMEMS = 2500;
const float DEPLOYDISTANCE = 30.0f;
const UInt32 STINGER_ANIM = 54; // 66

const UInt32 STATES_DEPLY_STGR = 0x39;

UInt32 NumOfStingerSegments;

class CSettings
{
public:
	char m_nStingerModelName[256];
	UInt32 m_nCopWithStingerChance;		
	UInt32 m_nMaxStingers;		
	UInt32 m_nDeployTimeMs;		
	float m_fDeployDistance;		
	UInt32 m_nAnim;		

	CSettings()
	{
		CIniReader Ini("");
		
		char *pGameVersion = Ini.ReadString("MAIN", "Version", "auto");
		for ( Int n = 1; n < ARRLEN(aGameVer); n++)
		{
			if (!strcmp(pGameVersion, aGameVer[n]) && strcmp(pGameVersion, "auto"))
			{
				bAutoVersionCheck = false;
				nVersion = n;
				break;
			}
		}
		if (!strcmp(pGameVersion, "auto"))
		{
			bAutoVersionCheck = true;
			nVersion = -1;
		}
				
		strncpy(m_nStingerModelName, Ini.ReadString("MAIN", "StingerModelName", STINGER_MODEL_NAME), sizeof(m_nStingerModelName));
		
		m_nCopWithStingerChance = Ini.ReadInteger("MAIN", "CopWithStingerChance", COP_WITH_STINGER_CHANCE) - 1;
		m_nMaxStingers = Ini.ReadInteger("MAIN", "MaxStingers", MAX_STINGERS);
		m_nDeployTimeMs = Ini.ReadInteger("MAIN", "DeployTimeMs", DEPLOYTIMEMS);
		m_fDeployDistance = Ini.ReadFloat("MAIN", "MinDeployDistance", DEPLOYDISTANCE);
		m_nAnim = Ini.ReadInteger("MAIN", "Anim", STINGER_ANIM);
	}
	
}settings;

Bool __cdecl CAN_THROWS_STINGER_Func(CPed *ped)
{
	if ( ped && ped->m_nPedType == PEDTYPE_COP )
		return true;
	
	return false;
}

typedef Bool ( __cdecl *tCAN_THROWS_STINGERCB)(CPed *ped);
tCAN_THROWS_STINGERCB CAN_THROWS_STINGERCB = CAN_THROWS_STINGER_Func;

Bool CAN_THROWS_STINGER(CPed *ped)
{
	return CAN_THROWS_STINGERCB(ped);
}

int _cwrand()
{
	return ((int (__cdecl *)())AddressByVersion(0x5A41D0, 0X5A4490, 0X5A5170))();
}

CBaseModelInfo *getModelInfoPtrs(unsigned int id)
{
	//CBaseModelInfo **CModelInfo::ms_modelInfoPtrs = (CBaseModelInfo **)AddressByVersion(0x83D408, 0x83D408, 0x84D548);
	
	CBaseModelInfo ***_modelInfoPtrs = (CBaseModelInfo ***)(AddressByVersion(0x50B870, 0x50B960, 0x50B8F0) + 3);
		
	return (*_modelInfoPtrs)[id];
}

static int destroy_array_addr = AddressByVersion(0x5A0620, 0x5A08E0, 0x5A0A40);
template<typename AT, typename FT> inline void
destroy_array(AT mem, FT f)
{
	_asm{
		push	f
		push	mem
		call	destroy_array_addr
		add	esp, 8
	}
}

enum eBikeNodes {
	BIKE_NODE_NONE,
	BIKE_CHASSIS,
	BIKE_FORKS_FRONT,
	BIKE_FORKS_REAR,
	BIKE_WHEEL_FRONT,
	BIKE_WHEEL_REAR,
	BIKE_MUDGUARD,
	BIKE_HANDLEBARS,
	BIKE_NUM_NODES
};

class CBike
{
public:
	char unk[0x314];
	UInt8 m_wheelStatus[2];
	CColPoint m_sWheelColPoint[4];
	float m_aSuspensionSpringRatio[4];
	float m_afWheelSuspDistSoft[4];
};

enum eStingerStatus
{
	STINGERSTATE_NONE = 0,
	STINGERSTATE_DEPLOYING,
	STINGERSTATE_DEPLOYED,
	STINGERSTATE_UNDEPLOYING,
	STINGERSTATE_REMOVE,
};

class CStingerSegment
{
protected:
	char _Data[sizeof(CObject)];
public:
	void ctor();
	void dtor();
	
	void *virtual_destructor(char flag);
};

class CStinger
{
public:
	Bool m_bActive;
	Int32 m_nTimer;
	CVector m_vecPosition;
	Float m_fAngle;
	CVector2D m_vec2dAngleTable[60];
	CStingerSegment *m_apSegments[MAX_STINGER_SEGMENTS];
	CPed *m_pOwner;
	Int8 m_nStingerState;
  
	CStinger()
	{
		m_bActive = false;
		
		m_nTimer = 0;
		
		for ( Int32 i = 0; i < MAX_STINGER_SEGMENTS; i++ )
			m_apSegments[i] = NULL;
		
		m_pOwner = NULL;
		
		m_nStingerState = STINGERSTATE_NONE;
	}

	~CStinger()
	{ }
	
	CObject *GetSegment(int index)
	{
		return (CObject *)m_apSegments[index];
	}
	
	bool Init(CPed *pCop);
	void Remove();	
	void Process();
	void CheckForBurstTyres();
	void Deploy(CPed *pPed);
	void _Animate(Float t);
};


class StingerPedEx
{
public:
	class CStinger *m_pStinger;
	Bool m_bProcessStinger;

	StingerPedEx()
	{
		;
	}
	
	StingerPedEx(CPed *ped)
	{
		m_pStinger = new CStinger();
		m_bProcessStinger = false;
	}
	
	~StingerPedEx()
	{
		if ( m_pStinger )
		{
			m_pStinger->Remove();
			delete m_pStinger;
			m_pStinger = NULL;
		}
		
		m_bProcessStinger = false;
	}

	void Store(bool before)
	{
		if ( !before )
		{
			if ( m_pStinger )
				m_pStinger = NULL;
			m_bProcessStinger = false;
		}
	}
	
	void Restore()
	{
		
	}
};

static PedExtendedData<StingerPedEx > StingerEx;

#define DEF_VMTCALL(n, a) \
	static int n##_addr = a; \
	void NAK n() { VARJMP(n##_addr); }
	

DEF_VMTCALL(VMT_Add,							AddressByVersion(0x4951F0, 0x4952B0, 0x495240))
DEF_VMTCALL(VMT_Remove,							AddressByVersion(0x4954B0, 0x495570, 0x495500))
DEF_VMTCALL(VMT_SetModelIndex,					AddressByVersion(0x473E70, 0x473E70, 0x473E70))
DEF_VMTCALL(VMT_SetModelIndexNoCreate,			AddressByVersion(0x473E90, 0x473E90, 0x473E90))
DEF_VMTCALL(VMT_CreateRwObject,					AddressByVersion(0x473EA0, 0x473EA0, 0x473EA0))
DEF_VMTCALL(VMT_DeleteRwObject,					AddressByVersion(0x473F90, 0x473F90, 0x473F90))
DEF_VMTCALL(VMT_GetBoundRect,					AddressByVersion(0x495150, 0x495210, 0x4951A0))
DEF_VMTCALL(VMT_ProcessControl,					AddressByVersion(0x4BB040, 0x4BB130, 0x4BB0C0))
DEF_VMTCALL(VMT_ProcessCollision,				AddressByVersion(0x4961A0, 0x496260, 0x4961F0))
DEF_VMTCALL(VMT_ProcessShift,					AddressByVersion(0x496F10, 0x496FD0, 0x496F60))
DEF_VMTCALL(VMT_Teleport,						AddressByVersion(0x4BBDA0, 0x4BBE90, 0x4BBE20))
DEF_VMTCALL(VMT_PreRender,						AddressByVersion(0x474350, 0x474350, 0x474350))
DEF_VMTCALL(VMT_Render,							AddressByVersion(0x4BB1E0, 0x4BB2D0, 0x4BB260))
DEF_VMTCALL(VMT_SetupLighting,					AddressByVersion(0x4A7C90, 0x4A7D80, 0x4A7D10))
DEF_VMTCALL(VMT_RemoveLighting,					AddressByVersion(0x4A7CD0, 0x4A7DC0, 0x4A7D50))
DEF_VMTCALL(VMT_FlagToDestroyWhenNextProcessed,	AddressByVersion(0x405940, 0x405940, 0x405940))
DEF_VMTCALL(VMT_ProcessEntityCollision,			AddressByVersion(0x49F790, 0x49F880, 0x49F810))

void NAK VMT_dtor()
{
	__asm jmp CStingerSegment::virtual_destructor
}

void *vtable_for_CStingerSegment[] = 
{
	(void*)VMT_dtor,
	(void*)VMT_Add,											//CPhysical::Add((void))
	(void*)VMT_Remove,										//CPhysical::Remove((void))
	(void*)VMT_SetModelIndex,								//CEntity::SetModelIndex((uint))
	(void*)VMT_SetModelIndexNoCreate,						//CEntity::SetModelIndexNoCreate((uint))
	(void*)VMT_CreateRwObject,								//CEntity::CreateRwObject((void))
	(void*)VMT_DeleteRwObject,								//CEntity::DeleteRwObject((void))
	(void*)VMT_GetBoundRect,								//CPhysical::GetBoundRect((void))
	(void*)VMT_ProcessControl,								//CObject::ProcessControl((void))
	(void*)VMT_ProcessCollision,							//CPhysical::ProcessCollision((void))
	(void*)VMT_ProcessShift,								//CPhysical::ProcessShift((void))
	(void*)VMT_Teleport,									//CObject::Teleport((CVector))
	(void*)VMT_PreRender,									//CEntity::PreRender((void))
	(void*)VMT_Render,										//CObject::Render((void))
	(void*)VMT_SetupLighting,								//CObject::SetupLighting((void))
	(void*)VMT_RemoveLighting,								//CObject::RemoveLighting((bool))
	(void*)VMT_FlagToDestroyWhenNextProcessed,				//CEntity::FlagToDestroyWhenNextProcessed((void))
	(void*)VMT_ProcessEntityCollision,						//CPhysical::ProcessEntityCollision((CEntity *,CColPoint *))
	
	NULL, NULL
};

void CStingerSegment::ctor()
{	
	CObject *This = (CObject *)this;

	((void (__thiscall *)(CObject *))AddressByVersion(0x4BABD0, 0x4BACC0, 0x4BAC50))(This);
	
	SETVMT(vtable_for_CStingerSegment);
	
	This->m_fMass = 1.0f;
	This->m_fTurnMass = 1.0f;
	This->m_fAirResistance = 0.99999f;
	This->m_fElasticity = 0.75;
	This->m_fBuoyancyConstant = 0.008f * This->m_fMass * 0.1f;
	This->m_bExplosionProof = true;

	int mi;

	if ( CModelInfo::GetModelInfo(settings.m_nStingerModelName, &mi) )
		This->SetModelIndex(mi); //MI_PLC_STINGER
	else
	{
		if ( CModelInfo::GetModelInfo("trafficcone", &mi) )
			This->SetModelIndex(mi);
		else
			This->SetModelIndex(1);
	}
	
	This->m_nObjectType = 5;
	
	++NumOfStingerSegments;
}
	
void CStingerSegment::dtor()
{	
	SETVMT(vtable_for_CStingerSegment);
	--NumOfStingerSegments;

	CObject *This = (CObject *)this;
	
	((void (__thiscall *)(CObject *))AddressByVersion(0x4BAE00, 0x4BAEF0, 0x4BAE80))(This);
}

void *CStingerSegment::virtual_destructor(char flag)
{
	if(this)
	{
		if(flag & 2)
		{			
			int p = NULL;
			__asm mov p, offset CStingerSegment::dtor
			destroy_array(this, p);
		}
		else
		{			
			this->dtor();
			
			if(flag & 1)
				CObject::operator delete(this);
		}
	}
	
	return this;
}
	
bool CStinger::Init(CPed *pCop)
{
	m_pOwner = (CCopPed *)pCop;
	
	for ( Int32 i = 0; i < MAX_STINGER_SEGMENTS; i++ )
	{
		m_apSegments[i] = (CStingerSegment *)CObject::operator new(sizeof(CStingerSegment));
		if (!m_apSegments[i])  
		{
			Remove(); // Abort!! Pool is full
			return false;
		}
		
		m_apSegments[i]->ctor();
		GetSegment(i)->m_bUsesCollision = false;
	}
	
	m_bActive = true;
	m_vecPosition = pCop->m_sCoords.pos;
	m_vecPosition.z -= 1.0f;

	m_fAngle = DEG2RAD(90.0f) + atan2(-pCop->m_sCoords.up.x, pCop->m_sCoords.up.y);
	
	for ( Int32 i = 0; i < MAX_STINGER_SEGMENTS; i++ )
	{
		CVector pos = GetSegment(i)->m_sCoords.pos;
		GetSegment(i)->m_sCoords.SetRotate(0.0f, 0.0f, atan2(-pCop->m_sCoords.up.x, pCop->m_sCoords.up.y));
		GetSegment(i)->m_sCoords.pos += pos;
		GetSegment(i)->m_sCoords.pos = m_vecPosition;
	}
	
	CVector2D up(pCop->m_sCoords.up.x, pCop->m_sCoords.up.y);

	for ( Int32 i = 0; i < ARRAY_SIZE(m_vec2dAngleTable); i++ )
		m_vec2dAngleTable[i] = up * sinf(DEG2RAD(Float(i))) * 1.8f;
	
	m_nStingerState = STINGERSTATE_NONE;
	m_nTimer = CTimer::m_snTimeInMilliseconds;
	
	return true;
}

void CStinger::Remove()
{
	for ( Int32 i = 0; i < MAX_STINGER_SEGMENTS; i++  )
	{
		if ( m_apSegments[i] )
		{
			if (m_nStingerState != STINGERSTATE_NONE)
				GetSegment(i)->m_bDestroyWhenNextProcessed = true;
			else
				((void (__thiscall *)(CStingerSegment *, char))(*(void ***)m_apSegments[i])[0])(m_apSegments[i], 1);
			
			m_apSegments[i] = NULL;
		}
	}
	
	m_bActive = false;
}
	
void CStinger::Process()
{
	switch ( m_nStingerState )
	{
		case STINGERSTATE_NONE:
		{
			if ( m_pOwner
				&& !m_pOwner->m_bInVehicle
				&& m_pOwner->m_nPedState == STATES_DEPLY_STGR
				&& RpAnimBlendClumpGetAssociation(m_pOwner->m_pRwClump, settings.m_nAnim)->m_fCurrentTime > 0.39f )
					
			{
				m_nStingerState = STINGERSTATE_DEPLOYING;
				
				for ( Int32 i = 0; i < MAX_STINGER_SEGMENTS; i++ )
					CWorld::Add(GetSegment(i));

				m_pOwner->SetIdle();
			}
			break;
		}
			
		case STINGERSTATE_DEPLOYED:
		{
			if ( CAN_THROWS_STINGER(m_pOwner) )
				StingerEx.Get((CPed *)m_pOwner).m_bProcessStinger = false;
			break;
		}

		case STINGERSTATE_UNDEPLOYING:
		{
			if ( CTimer::m_snTimeInMilliseconds > m_nTimer + settings.m_nDeployTimeMs )
				m_nStingerState = STINGERSTATE_REMOVE;
		}
		
		case STINGERSTATE_DEPLOYING:
		{
			if ( m_nStingerState == STINGERSTATE_DEPLOYING && CTimer::m_snTimeInMilliseconds > m_nTimer + settings.m_nDeployTimeMs )
				m_nStingerState = STINGERSTATE_DEPLOYED;
			else
				_Animate(Float(CTimer::m_snTimeInMilliseconds - m_nTimer) / Float(settings.m_nDeployTimeMs) );
			break;
		}
		
		case STINGERSTATE_REMOVE:
		{
			Remove();
			return;
			break;
		}
	}

	CheckForBurstTyres();
}
	
void CStinger::CheckForBurstTyres()
{
	CVector vStart = GetSegment(0)->m_sCoords.pos;
	vStart.z += 0.2f;
	
	CVector vEnd = GetSegment(MAX_STINGER_SEGMENTS-1)->m_sCoords.pos;
	vEnd.z += 0.2f;
	
	Float fLength = (vEnd - vStart).Magnitude();
	
	if ( fLength < 0.1f )
		return;
	
	CVector vPos = (vStart + vEnd) * 0.5f;
	
	Int16 nCount;
	CEntity *pEntities[16];
	
	CWorld::FindObjectsInRange(vPos, fLength, true, &nCount, 15, pEntities, false, true, false, false, false);
	
	for ( Int32 i = 0; i < nCount; i++ )
	{
		CAutomobile *veh = NULL;
		CBike *pBike = NULL;
		
		if ( ((CVehicle *)pEntities[i])->m_eVehicleType == VEHICLETYPE_CAR )
			veh = (CAutomobile *)pEntities[i];
		else if (((CVehicle *)pEntities[i])->m_eVehicleType == VEHICLETYPE_BIKE)
			pBike = (CBike*)pEntities[i];
		
		if ( veh == NULL && pBike == NULL)
			continue;
		
		CVehicleModelInfo *vehModel = (CVehicleModelInfo *)getModelInfoPtrs(pEntities[i]->m_nModelIndex);
		
		Float fWheelScale = vehModel->m_fWheelScale + 0.1f;
		Float fWheelScaleSqr = SQR(fWheelScale);
		
		for ( Int32 j = 0; j < 4; j++ )
		{
			if ( (veh != NULL && veh->m_afWheelSuspDistSoft[j] < 1.0f) ||
				(pBike != NULL && pBike->m_afWheelSuspDistSoft[j] < 1.0f) )
			{
				CVector vPoint;
				
				if (veh != NULL)
					vPoint = veh->m_sWheelColPoint[j].m_vecPosition;
				else if (pBike != NULL)
					vPoint = pBike->m_sWheelColPoint[j].m_vecPosition;
				
				for ( Int32 k = 0; k < MAX_STINGER_SEGMENTS; k++ )
				{
					CVector vDist = GetSegment(k)->m_sCoords.pos - vPoint;
					if ( vDist.Magnitude() < fWheelScaleSqr )
					{
						if ( pBike )
						{
							if (j < 2)
								((CAutomobile *)pBike)->BurstTyre(13);
							else
								((CAutomobile *)pBike)->BurstTyre(15);
						}
						else
						{
							switch ( j )
							{
								case 0:
									veh->BurstTyre(13);
									break;
								case 1:
									veh->BurstTyre(15);
									break;
								case 2:
									veh->BurstTyre(14);
									break;
								case 3:
									veh->BurstTyre(16);
									break;
							}
						}
						
						vPoint.z += 0.15f;

						for ( Int32 l = 0; l < 4; l++ )
							CParticle::AddParticle(PARTICLE_BULLETHIT_SMOKE, vPoint, 0.1f * pEntities[i]->m_sCoords.right, NULL, 0.0f, 0, 0, 0, 0);
					}
				}
			}
		}
		
	}
}
	
void CStinger::Deploy(CPed *pPed)
{
	if ( NumOfStingerSegments < settings.m_nMaxStingers*MAX_STINGER_SEGMENTS && !pPed->m_bInVehicle )
	{
		if ( pPed->IsPedInControl() )
		{
			if ( !m_bActive && RpAnimBlendClumpGetAssociation(pPed->m_pRwClump, settings.m_nAnim) == NULL )
			{
				if ( Init(pPed) )
				{
					pPed->m_nPedState = (ePedStates)STATES_DEPLY_STGR;
					CAnimManager::AddAnimation(pPed->m_pRwClump, (AssocGroupId)0, (AnimationId)settings.m_nAnim);
				}
			}
		}
	}
}
	
void CStinger::_Animate(Float t)
{
	if (m_nStingerState != STINGERSTATE_DEPLOYING)
		t = 1.0f - t;
			
	Float fAngle = float(ARRAY_SIZE(m_vec2dAngleTable)) * t;
	Float fAngleRad = DEG2RAD(fAngle);

	Float fAngle1 = m_fAngle + fAngleRad;
	Float fAngle2 = m_fAngle - fAngleRad;

	Int32 nAngle = clamp(((Int32)fAngle), 0, ARRAY_SIZE(m_vec2dAngleTable)-1);	
	
	CVector2D calcAngle = m_vec2dAngleTable[nAngle];
	
	CVector pos = m_vecPosition;

	CColPoint point;
	CEntity *entity;
	if ( CWorld::ProcessVerticalLine(CVector(pos.x, pos.y, pos.z - 10.0f), pos.z, point, entity, true, false, false, false, true, false, NULL) )
		pos.z = 0.15f + point.m_vecPosition.z;
	
	for ( Int32 i = 0; i < MAX_STINGER_SEGMENTS; i++ )
	{		
		if ( CWorld::TestSphereAgainstWorld(CVector(pos.x + calcAngle.x, pos.y + calcAngle.y, pos.z + 0.6f), 0.3f, NULL, true, false, false, true, false, false) )
		{
			calcAngle.x = 0.0f;
			calcAngle.y = 0.0f;
		}

		if ((i % 2) == 0)
		{
			CVector savedPos = GetSegment(i)->m_sCoords.pos;	
			GetSegment(i)->m_sCoords.SetRotate(0.0f, 0.0f, CGeneral::LimitRadianAngle(fAngle1));	
			GetSegment(i)->m_sCoords.pos += savedPos;
			
			pos.x += calcAngle.x;
			pos.y += calcAngle.y;
		}
		else
		{
			CVector savedPos = GetSegment(i)->m_sCoords.pos;
			GetSegment(i)->m_sCoords.SetRotate(0.0f, 0.0f, CGeneral::LimitRadianAngle(fAngle2));
			GetSegment(i)->m_sCoords.pos += savedPos;
		}
		

		GetSegment(i)->m_sCoords.pos = pos;
	}

}

class CCopPedEx : public CCopPed
{
public:
	Bool ProcessCtrol();
	void ProcessStingerCop();
};

Bool CCopPedEx::ProcessCtrol()
{
	CStinger *&m_pStinger = StingerEx.Get((CPed *)this).m_pStinger;
	Bool &m_bProcessStinger = StingerEx.Get((CPed *)this).m_bProcessStinger;
	
	if ( !m_pStinger )
		return false;

	if ( m_bProcessStinger )
	{
		ProcessStingerCop();
		return true;
	}

	if ( m_pStinger && m_pStinger->m_bActive && m_pStinger->m_nStingerState == STINGERSTATE_DEPLOYED )
		m_pStinger->Process();

	return false;
}

void CCopPedEx::ProcessStingerCop()
{
	CStinger *&m_pStinger = StingerEx.Get((CPed *)this).m_pStinger;

	if ( !m_pStinger )
		return;
	
	if ( m_pStinger->m_bActive || ( FindPlayerVehicle() && (FindPlayerVehicle()->m_eVehicleType == VEHICLETYPE_CAR || FindPlayerVehicle()->m_eVehicleType == VEHICLETYPE_BIKE) ) )
	{
		if ( m_pStinger->m_bActive )
			m_pStinger->Process();
		else
		{
			CVector2D dist(m_sCoords.pos.x - FindPlayerVehicle()->m_sCoords.pos.x, m_sCoords.pos.y - FindPlayerVehicle()->m_sCoords.pos.y);
			CVector2D speed(FindPlayerVehicle()->m_vecVelocity.x, FindPlayerVehicle()->m_vecVelocity.y);
			Float distSqr = dist.MagnitudeSqr();
			Float speedSqr = speed.MagnitudeSqr();
			
			if ( distSqr < SQR(settings.m_fDeployDistance) && speedSqr > 0.0f )
			{
				dist.Normalise();
				speed.Normalise();

				if ( DotProduct(dist, speed) > 0.8f )
				{
					float rot = (CrossProduct2D(dist, speed - dist) < 0.0f ?
							FindPlayerVehicle()->GetForward().Heading() - HALFPI :
							HALFPI + FindPlayerVehicle()->GetForward().Heading());

					SetHeading(rot);						
					m_fCurrentRotation = rot;
					m_fAimingRotation = rot;
					m_pStinger->Deploy(this);
				}
			}
		}
	}
	else
		ClearPursuit();
}

static int jmp_0x4C1446 = AddressByVersion(0x4C1446, 0x4C1536, 0x4C14C6);
static int jmp_0x4C143E = AddressByVersion(0x4C143E, 0x4C152E, 0x4C14BE);
void NAK CCopPedProc()
{
	__asm
	{
		mov     ecx, ebx
		call    CCopPedEx::ProcessCtrol
		
		test    al, al
		jz	    _CPC_ORIGINAL_CODE
		
		jmp     _CPC_RET

	}	
	
_CPC_ORIGINAL_CODE:
	__asm
	{
		mov     al, byte ptr [ebx+52h]
		and     al, 1
		jz      short loc_4C1446
		
		jmp     _CPC_RET
	}

	
loc_4C1446:
	VARJMP(jmp_0x4C1446);
	
_CPC_RET:
	VARJMP(jmp_0x4C143E);
}


Bool bCreateCopWithStinger = false;

void SetupCopStinger()
{
	bCreateCopWithStinger = false;
	
	if ( FindPlayerPed()->m_pWanted->m_nWantedLevel > 2 )
	{
		if ( !(_cwrand() & settings.m_nCopWithStingerChance) && FindPlayerVehicle() )
			bCreateCopWithStinger = true;
	}	
}

static int jmp_0x4F4A18 = AddressByVersion(0x4F4A18, 0x4F4AC8, 0x4F4A58);
void NAK AddToPopulationPatchInit()
{
	__asm
	{
		mov     byte ptr [esp+100h-0E8h], 0		
		pushad
		call     SetupCopStinger
		popad
	
	}
	VARJMP(jmp_0x4F4A18);
}


static int jmp_0x4F4B0D = AddressByVersion(0x4F4B0D, 0x4F4BBD, 0x4F4B4D);
static int jmp_0x4F4B36 = AddressByVersion(0x4F4B36, 0x4F4BE6, 0x4F4B76);
static int jmp_0x4F4AA0 = AddressByVersion(0x4F4AA0, 0x4F4B50, 0x4F4AE0);

void NAK CheckStinger()
{
	__asm
	{		
		cmp     bCreateCopWithStinger, 0
		jz      _STINGER_FAIL
		
		jmp     _STINGER_OK
	}

_STINGER_OK:
	VARJMP(jmp_0x4F4B0D);
	
_STINGER_FAIL:
	VARJMP(jmp_0x4F4B36);
}

void NAK CheckStinger2()
{
	__asm
	{		
		cmp     bCreateCopWithStinger, 0
		jz      _STINGER_FAIL
		
		jmp     _STINGER_OK
	}

_STINGER_OK:
	VARJMP(jmp_0x4F4AA0);
	
_STINGER_FAIL:
	VARJMP(jmp_0x4F4B36);
}

int &MaxNumberOfCarsInUse = *(int*)AddressByVersion(0x5EC8B8, 0x5EC8B8, 0x5F98B8);
void NAK AddToPopulationPatch()
{
	__asm
	{
		push    ecx
		
		mov     ecx, dword ptr MaxNumberOfCarsInUse
		mov     ecx, [ecx]
		cmp     eax, ecx
		
		pop     ecx

		jge     loc_4F4B0D
		
loc_53BC49:
		jmp     CheckStinger
	}
	
loc_4F4B0D:				//create
	VARJMP(jmp_0x4F4B0D);
	
loc_4F4B36:				//ignore
	VARJMP(jmp_0x4F4B36);
}

void CopThrowsSpikeTrap(CPed *cop)
{
	if ( bCreateCopWithStinger && CAN_THROWS_STINGER(cop) )
		StingerEx.Get((CPed *)cop).m_bProcessStinger = true;
}

static int jmp_0x4F5171 = AddressByVersion(0x4F5171, 0x4F5221, 0x4F51B1);
void NAK AddPedPatch()
{
	__asm
	{
		add     esp, 0Ch
		mov     ebx, eax
		
		pushad
		push     ebx
		call     CopThrowsSpikeTrap
		pop      ebx
		popad
		
		jmp     jmp_0x4F5171
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		StingerEx.Init();

		CPatch::RedirectJump(AddressByVersion(0x4C1437, 0x4C1527, 0x4C14B7), CCopPedProc);
		
		CPatch::RedirectJump(AddressByVersion(0x4F4A13, 0x4F4AC3, 0x4F4A53), AddToPopulationPatchInit);
		CPatch::RedirectJump(AddressByVersion(0x4F4B05, 0x4F4BB5, 0x4F4B45), AddToPopulationPatch);
		CPatch::RedirectJNZ(AddressByVersion(0x4F4AA7, 0x4F4B57, 0x4F4AE7), CheckStinger);
		CPatch::RedirectJGE(AddressByVersion(0x4F4A9A, 0x4F4B4A, 0x4F4ADA), CheckStinger2);
		
		CPatch::RedirectJump(AddressByVersion(0x4F516C, 0x4F521C, 0x4F51AC), AddPedPatch);
		
		
		/*m_nObjectType = 5*/
		static void *CanBeDeletedSwitchTable[] =
		{
			(void*)AddressByVersion(0x4BB030, 0x4BB120, 0x4BB0B0),
			(void*)AddressByVersion(0x4BB023, 0x4BB113, 0x4BB0A3),
			(void*)AddressByVersion(0x4BB026, 0x4BB116, 0x4BB0A6),
			(void*)AddressByVersion(0x4BB029, 0x4BB119, 0x4BB0A9),
			(void*)AddressByVersion(0x4BB02C, 0x4BB11C, 0x4BB0AC),
			(void*)AddressByVersion(0x4BB02C, 0x4BB11C, 0x4BB0AC),
			(void*)0,
			(void*)0
		};

		CPatch::SetChar(AddressByVersion(0x4BB017, 0x4BB107, 0x4BB097) + 2, 5);
		CPatch::SetPointer(AddressByVersion(0x4BB01C, 0x4BB10C, 0x4BB09C) + 3, CanBeDeletedSwitchTable);
		//
	}
	return TRUE;
}

namespace Stingers
{
	EXP StingerPedEx &GetStingerPedData(CPed *ped)
	{
		return StingerEx.Get(ped);
	}
	
	void EXP Set_CAN_THROWS_STINGER_CallBack(tCAN_THROWS_STINGERCB func)
	{
		CAN_THROWS_STINGERCB = func;
	}
	
	tCAN_THROWS_STINGERCB EXP Get_CAN_THROWS_STINGER_CallBack()
	{
		return CAN_THROWS_STINGERCB;
	}
};