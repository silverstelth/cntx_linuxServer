#pragma once

typedef	bool	(*TGetCharacterInfo)(void * arg, T_CHARACTERID _chid, T_CH_STATE& _State, T_CH_DIRECTION& _Dir, T_CH_X& _X, T_CH_Y& _Y, T_CH_Z& _Z, T_LEVEL& _Level);

enum TMoveFlag {
	MOVEFLAG_FLEE,
	MOVEFLAG_APPROACH,
};

struct MoveObject {
	bool			valid;
	T_CHARACTERID	chid;
	T_LEVEL			level;
	TMoveFlag		flag;		// flag: 1 for moving approach to player, 0 for escape from player
	T_CH_X			des_X; 
	T_CH_Y			des_Y;
	T_CH_Z			des_Z;
};

class CAnimal : public SIPBASE::CRefCount
{
public:
	T_COMMON_ID			m_ID;			
	T_ANIMALID			m_TypeID;		
	T_POS				m_PosX, m_PosY, m_PosZ;			
//T	T_CH_DIRECTION		m_Direction;

	static	uint		m_nMoveStatistic;	// 0 - 100 : percent
	static	T_LEVEL		m_nLevelTh;
	static	T_DISTANCE	m_TriggerMinDist, m_TriggerMaxDist;

protected:
	enum	TAnimalState {
		ANIMALSTATE_STOP,
		ANIMALSTATE_MOVE,
	};
	TAnimalState		m_State;
	SIPBASE::TTime		m_NextMoveTime;
	MoveObject          m_MoveObject;
	T_CHARACTERID		m_TargetChId;

	REGIONSCOPE *		m_pScope;
	CCellManager *		m_pCell;
	OUT_ANIMALPARAM *	m_pParam;

	TGetCharacterInfo	m_cbCh;
	void *				m_CbArg;

	CCellManager *		getCell()	{ return m_pCell; }
	bool	getCharacterInfo(T_CHARACTERID _chid, T_CH_STATE& _State, T_CH_DIRECTION& _Dir, T_CH_X& _X, T_CH_Y& _Y, T_CH_Z& _Z, T_LEVEL& _Level);

public:
	virtual	void	onCharacterMove(T_CHARACTERID chid);
	virtual	void	onUpdate(SIPBASE::TTime curtime);

	bool	init(T_ANIMALID type, T_COMMON_ID id, REGIONSCOPE * scope);
	void	setCharacterCallback(TGetCharacterInfo cb, void * arg)	{ m_cbCh = cb, m_CbArg = arg; }
	void	setCell(CCellManager * pCell) { m_pCell = pCell; }
	void	move(T_POSX x, T_POSZ z);

public:
	CAnimal();
	virtual	~CAnimal();
};

class CRabbit : public CAnimal
{
public:
	virtual	void	onCharacterMove(T_CHARACTERID chid);
	virtual	void	onUpdate(SIPBASE::TTime curtime);

public:
	CRabbit();
	~CRabbit();
};

typedef SIPBASE::CSmartPtr<CAnimal>			PAnimal;
typedef	std::map<T_COMMON_ID, PAnimal>		TAnimals;
