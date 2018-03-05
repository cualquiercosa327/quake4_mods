#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"
#include "../client/ClientEffect.h"
#include "../Projectile.h"
#include "../ai/AI_Manager.h"

const int	LIGHTNINGGUN_NUM_TUBES	=	3;
const int	LIGHTNINGGUN_MAX_PATHS  =	3;

const idEventDef EV_Lightninggun_RestoreHum( "<lightninggunRestoreHum>", "" );

class rvWeaponLightningGun : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponLightningGun );

	rvWeaponLightningGun( void );
	~rvWeaponLightningGun( void );

	virtual void			Spawn		( void );
	virtual void			Think		( void );

	virtual void			ClientStale	( void );
	virtual void			ClientUnstale( void );

	void					PreSave		( void );
	void					PostSave	( void );

	void					Save		( idSaveGame* savefile ) const;
	void					Restore		( idRestoreGame* savefile );

	bool					NoFireWhileSwitching( void ) const { return true; }

protected:

	rvClientEffectPtr					hookEffect;
	jointHandle_t						chestJointView;

private:

	void				Attack					( );

	void				UpdateTrailEffect		( rvClientEffectPtr& effect, const idVec3& start, const idVec3& end, bool view = false );

	stateResult_t		State_Raise				( const stateParms_t& parms );
	stateResult_t		State_Lower				( const stateParms_t& parms );
	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_Fire				( const stateParms_t& parms );

	idVec3								origin;
	idVec3								hookPoint;
	idVec3								hookVelocity;

	float								hookDesiredDist;

	void				Event_RestoreHum	( void );

	CLASS_STATES_PROTOTYPE ( rvWeaponLightningGun );
};

CLASS_DECLARATION( rvWeapon, rvWeaponLightningGun )
EVENT( EV_Lightninggun_RestoreHum,			rvWeaponLightningGun::Event_RestoreHum )
END_CLASS

/*
================
rvWeaponLightningGun::rvWeaponLightningGun
================
*/
rvWeaponLightningGun::rvWeaponLightningGun( void ) {
}

/*
================
rvWeaponLightningGun::~rvWeaponLightningGun
================
*/
rvWeaponLightningGun::~rvWeaponLightningGun( void ) {
	int i;
	
	if (hookEffect) {
		hookEffect->Stop();
	}
}

/*
================
rvWeaponLightningGun::Spawn
================
*/
void rvWeaponLightningGun::Spawn( void ) {
	int i;
	
	hookEffect = NULL;

	if( gameLocal.GetLocalPlayer())	{
		chestJointView = gameLocal.GetLocalPlayer()->GetAnimator()->GetJointHandle( spawnArgs.GetString ( "joint_hideGun_flash" ) );
	} else {
		chestJointView = viewModel->GetAnimator ( )->GetJointHandle ( "spire_1" );	
	}
	
	SetState ( "Raise", 0 );
}

/*
================
rvWeaponLightningGun::Save
================
*/
void rvWeaponLightningGun::Save	( idSaveGame* savefile ) const {
}

/*
================
rvWeaponLightningGun::Restore
================
*/
void rvWeaponLightningGun::Restore ( idRestoreGame* savefile ) {
}

/*
================
rvWeaponLightningGun::Think
================
*/
void rvWeaponLightningGun::Think ( void ) {
	rvWeapon::Think();

	if ( !wsfl.attack || !IsReady() ) {
		if (hookEffect) {
			hookEffect->Stop();
		}

		return;
	}

	idMat3 axis;
	GetGlobalJointTransform( true, chestJointView, origin, axis );

	if (hookEffect) {
		UpdateTrailEffect(hookEffect, origin, hookPoint);
	}
}

/*
================
rvWeaponLightningGun::Attack
================
*/
void rvWeaponLightningGun::Attack ( ) {
	// JOS: Trace the player's view to find the hook position
	trace_t tr;
	float range = 5000.0f;
	gameLocal.TracePoint(	owner, tr, 
							playerViewOrigin, 
							playerViewOrigin + playerViewAxis[0] * range, 
							(MASK_SHOT_RENDERMODEL|CONTENTS_WATER|CONTENTS_PROJECTILE), owner );

	hookPoint = tr.endpos;
	hookDesiredDist = hookPoint.Dist(gameLocal.GetLocalPlayer()->GetPlayerPhysics()->GetOrigin());
}

/*
================
rvWeaponLightningGun::UpdateTrailEffect
================
*/
void rvWeaponLightningGun::UpdateTrailEffect( rvClientEffectPtr& effect, const idVec3& start, const idVec3& end, bool view ) {
	idVec3 dir;
	dir = end - start;
	dir.Normalize();
	
	if ( !effect ) {
		effect = gameLocal.PlayEffect( gameLocal.GetEffect( weaponDef->dict, "fx_trail" ), start, dir.ToMat3(), true, end );		
	} else {
		effect->SetOrigin( start );
		effect->SetAxis( dir.ToMat3() );
		effect->SetEndOrigin( end );
	}
}

/*
================
rvWeaponLightningGun::ClientStale
================
*/
void rvWeaponLightningGun::ClientStale( void ) {
	rvWeapon::ClientStale( );

	if ( hookEffect ) {
		hookEffect->Stop();
		hookEffect->Event_Remove();
		hookEffect = NULL;
	}
}

/*
================
rvWeaponLightningGun::PreSave
================
*/
void rvWeaponLightningGun::PreSave( void ) {
}

/*
================
rvWeaponLightningGun::PostSave
================
*/
void rvWeaponLightningGun::PostSave( void ) {
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponLightningGun )
	STATE ( "Raise",						rvWeaponLightningGun::State_Raise )
	STATE ( "Lower",						rvWeaponLightningGun::State_Lower )
	STATE ( "Idle",							rvWeaponLightningGun::State_Idle)
	STATE ( "Fire",							rvWeaponLightningGun::State_Fire )
END_CLASS_STATES

/*
================
rvWeaponLightningGun::State_Raise
================
*/
stateResult_t rvWeaponLightningGun::State_Raise( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	

	switch ( parms.stage ) {
		// Start the weapon raising
		case STAGE_INIT:
			SetStatus( WP_RISING );
			viewModel->SetShaderParm( 6, 1 );
			PlayAnim( ANIMCHANNEL_ALL, "raise", parms.blendFrames );
			return SRESULT_STAGE( STAGE_WAIT );
		
		// Wait for the weapon to finish raising and allow it to be cancelled by
		// lowering the weapon 
		case STAGE_WAIT:
			if ( AnimDone( ANIMCHANNEL_ALL, 4 ) ) {
				SetState( "Idle", 0 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				StopSound( SND_CHANNEL_BODY3, false );
				SetState( "Lower", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponLightningGun::State_Lower
================
*/
stateResult_t rvWeaponLightningGun::State_Lower( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
		STAGE_WAITRAISE
	};	

	switch ( parms.stage ) {
		case STAGE_INIT:
			SetStatus( WP_LOWERING );
			PlayAnim( ANIMCHANNEL_ALL, "putaway", parms.blendFrames );
			return SRESULT_STAGE(STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone( ANIMCHANNEL_ALL, 0 ) ) {
				SetStatus( WP_HOLSTERED );
				return SRESULT_STAGE(STAGE_WAITRAISE);
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITRAISE:
			if ( wsfl.raiseWeapon ) {
				SetState( "Raise", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponLightningGun::State_Idle
================
*/
stateResult_t rvWeaponLightningGun::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	

	switch ( parms.stage ) {
		case STAGE_INIT:
			SetStatus( WP_READY );
			PlayCycle( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
			StopSound( SND_CHANNEL_BODY3, false );
			StartSound( "snd_idle_hum", SND_CHANNEL_BODY3, 0, false, NULL );

			return SRESULT_STAGE( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( wsfl.lowerWeapon ) {
				StopSound( SND_CHANNEL_BODY3, false );
				SetState( "Lower", 0 );
				return SRESULT_DONE;
			}
			if ( wsfl.attack ) {
				Attack();
				SetState( "Fire", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponLightningGun::State_Fire
================
*/
stateResult_t rvWeaponLightningGun::State_Fire( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_HOOKED,
		STAGE_DONE
	};	

	auto player = gameLocal.GetLocalPlayer();
	auto phys = player->GetPlayerPhysics();

	float frametime = (gameLocal.time - gameLocal.previousTime) * 0.001f;
	idVec3 projectedPos = phys->GetOrigin() + phys->GetLinearVelocity() * frametime;

	idVec3 desiredPos;

	idVec3 newVel;
	auto diff = projectedPos - hookPoint;
	diff.Normalize();

	switch ( parms.stage ) {
		case STAGE_INIT:
			// StartSound( "snd_fire", SND_CHANNEL_WEAPON, 0, false, NULL );
			// StartSound( "snd_fire_stereo", SND_CHANNEL_ITEM, 0, false, NULL );
			// StartSound( "snd_fire_loop", SND_CHANNEL_BODY2, 0, false, NULL );
			
			viewModel->PlayEffect( "fx_flash", barrelJointView, true );

			PlayAnim( ANIMCHANNEL_ALL, "shoot_start", parms.blendFrames );

			return SRESULT_STAGE( STAGE_HOOKED );
		
		case STAGE_HOOKED:
			UpdateTrailEffect(hookEffect, playerViewOrigin, hookPoint, false);

			if (projectedPos.Dist(hookPoint) > hookDesiredDist) {
				desiredPos = hookPoint + diff * hookDesiredDist;
				newVel = desiredPos - phys->GetOrigin();

				// phys->SetOrigin(desiredPos);
				phys->SetLinearVelocity(newVel * (1.0f / frametime));
			}

			if (!wsfl.attack) {
				SetState("Idle", 0);

				return SRESULT_DONE;
			}

			return SRESULT_WAIT;
						
		case STAGE_DONE:
			// StopSound( SND_CHANNEL_BODY2, false );

			// viewModel->StopEffect( "fx_flash" );
			// viewModel->SetShaderParm( 6, 1 );

			// PlayAnim( ANIMCHANNEL_ALL, "shoot_end", 0 );
			return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}


/*
================
rvWeaponLightningGun::Event_RestoreHum
================
*/
void rvWeaponLightningGun::Event_RestoreHum ( void ) {
}

/*
================
rvWeaponLightningGun::ClientUnStale
================
*/
void rvWeaponLightningGun::ClientUnstale( void ) {
	Event_RestoreHum();
}