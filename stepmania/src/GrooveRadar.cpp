#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: GrooveRadar

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "RageUtil.h"

#include "GrooveRadar.h"
#include "PrefsManager.h"
#include "RageBitmapTexture.h"
#include "GameConstantsAndTypes.h"


float RADAR_VALUE_ROTATION( int iValueIndex ) {	return D3DX_PI/2 + D3DX_PI*2 / 5.0f * iValueIndex; }

const float RADAR_EDGE_WIDTH	= 3;

GrooveRadar::GrooveRadar()
{
	this->AddChild( &m_GrooveRadarValueMap );

	for( int c=0; c<NUM_RADAR_CATEGORIES; c++ )
	{
		const float fRadius = m_GrooveRadarValueMap.m_sprRadarBase.GetZoomedHeight()/2.0f;
		const float fRotation = RADAR_VALUE_ROTATION(c);
		float fX = cosf(fRotation) * fRadius;
		
		// push the labels out a little
		switch( c )
		{
		case 0:												break;
		case 1:
		case 4: if( fabsf(fX) > 1 ) fX += fX/fabsf(fX) * 40;	break;
		case 2:
		case 3: if( fabsf(fX) > 1 ) fX += fX/fabsf(fX) * 50;	break;
		default:	ASSERT( false );
		}

		const float fY = -sinf(fRotation) * fRadius * 1.15f;
		

		m_sprRadarLabels[c].Load( THEME->GetPathTo("Graphics","select music radar words") );
		m_sprRadarLabels[c].StopAnimating();
		m_sprRadarLabels[c].SetState( c );
		m_sprRadarLabels[c].SetXY( fX, fY );
		this->AddChild( &m_sprRadarLabels[c] );
	}
}

void GrooveRadar::TweenOnScreen()
{
	for( int c=0; c<NUM_RADAR_CATEGORIES; c++ )
	{
		float fOriginalX = m_sprRadarLabels[c].GetX();
		m_sprRadarLabels[c].SetX( fOriginalX - 100 );
		m_sprRadarLabels[c].SetZoom( 1.5f );
		m_sprRadarLabels[c].SetDiffuse( D3DXCOLOR(1,1,1,0) );

		m_sprRadarLabels[c].BeginTweening( 0.6f+0.2f*c );	// sleep

		m_sprRadarLabels[c].BeginTweening( 0.1f );	// begin fading on screen
		m_sprRadarLabels[c].SetTweenGlow( D3DXCOLOR(1,1,1,1) );
		
		m_sprRadarLabels[c].BeginTweening( 0.3f, Actor::TWEEN_BIAS_BEGIN );	// fly to the right
		m_sprRadarLabels[c].SetTweenX( fOriginalX );
		m_sprRadarLabels[c].SetTweenZoom( 1 );
		m_sprRadarLabels[c].SetTweenGlow( D3DXCOLOR(1,1,1,0) );
		m_sprRadarLabels[c].SetTweenDiffuse( D3DXCOLOR(1,1,1,1) );
	}
	m_GrooveRadarValueMap.TweenOnScreen();
}

void GrooveRadar::TweenOffScreen()
{
	for( int c=0; c<NUM_RADAR_CATEGORIES; c++ )
	{
		m_sprRadarLabels[c].StopTweening();
		m_sprRadarLabels[c].BeginTweening( 0.2f );
		m_sprRadarLabels[c].SetTweenDiffuse( D3DXCOLOR(1,1,1,0) );
	}
	m_GrooveRadarValueMap.TweenOffScreen();
}

GrooveRadar::GrooveRadarValueMap::GrooveRadarValueMap()
{
	m_sprRadarBase.Load( THEME->GetPathTo("Graphics","select music radar base") );
	this->AddChild( &m_sprRadarBase );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		m_bValuesVisible[p] = false;
		m_PercentTowardNew[p] = 0;

		for( int c=0; c<NUM_RADAR_CATEGORIES; c++ )
		{
			m_fValuesNew[p][c] = 0;
			m_fValuesOld[p][c] = 0;
		}
	}
}

void GrooveRadar::GrooveRadarValueMap::SetFromNotes( PlayerNumber p, Notes* pNotes )		// NULL means no song
{
	if( pNotes != NULL )
	{
		for( int c=0; c<NUM_RADAR_CATEGORIES; c++ )
		{
			const float fValueCurrent = m_fValuesOld[p][c] * (1-m_PercentTowardNew[p]) + m_fValuesNew[p][c] * m_PercentTowardNew[p];
			m_fValuesOld[p][c] = fValueCurrent;
			m_fValuesNew[p][c] = pNotes->m_fRadarValues[c];
		}

		if( m_bValuesVisible[p] == false )	// the values WERE invisible
			m_PercentTowardNew[p] = 1;
		else
			m_PercentTowardNew[p] = 0;

		m_bValuesVisible[p] = true;
	}
	else	// pNotes == NULL
	{
		m_bValuesVisible[p] = false;
	}
}

void GrooveRadar::GrooveRadarValueMap::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		m_PercentTowardNew[p] = min( m_PercentTowardNew[p]+4.0f*fDeltaTime, 1 );
	}
}

void GrooveRadar::GrooveRadarValueMap::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();

	// draw radar filling
	const float fRadius = m_sprRadarBase.GetZoomedHeight()/2.0f*1.1f;

	DISPLAY->SetTexture( NULL );
	DISPLAY->SetColorTextureMultDiffuse();
	DISPLAY->SetAlphaTextureMultDiffuse();
	RAGEVERTEX v[12];	// needed to draw 5 fan primitives and 10 strip primitives

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !m_bValuesVisible[p] )
			continue;

		//
		// use a fan to draw the volume
		//
		D3DXCOLOR color = PlayerToColor( (PlayerNumber)p );
		color.a = 0.5f;
		v[0].p = D3DXVECTOR3( 0, 0, 0 );
		v[0].color = color;

		int i;

		for( i=0; i<NUM_RADAR_CATEGORIES+1; i++ )	// do one extra to close the fan
		{
			const int c = i%NUM_RADAR_CATEGORIES;
			const float fDistFromCenter = 
				( m_fValuesOld[p][c] * (1-m_PercentTowardNew[p]) + m_fValuesNew[p][c] * m_PercentTowardNew[p] + 0.07f ) * fRadius;
			const float fRotation = RADAR_VALUE_ROTATION(i);
			const float fX = cosf(fRotation) * fDistFromCenter;
			const float fY = -sinf(fRotation) * fDistFromCenter;

			v[1+i].p = D3DXVECTOR3( fX, fY,	0 );
			v[1+i].color = v[0].color;
		}

		DISPLAY->AddFan( v, 5 );


		//
		// use a strip to draw the thick line
		//
		for( i=0; i<NUM_RADAR_CATEGORIES+1; i++ )	// do one extra to close the fan
		{
			const int c = i%NUM_RADAR_CATEGORIES;
			const float fDistFromCenter = 
				( m_fValuesOld[p][c] * (1-m_PercentTowardNew[p]) + m_fValuesNew[p][c] * m_PercentTowardNew[p] + 0.07f ) * fRadius;
			const float fDistFromCenterInner = fDistFromCenter-RADAR_EDGE_WIDTH/2;
			const float fDistFromCenterOutter = fDistFromCenter+RADAR_EDGE_WIDTH/2;
			const float fRotation = RADAR_VALUE_ROTATION(i);
			const float fXInner = cosf(fRotation) * fDistFromCenterInner;
			const float fXOutter = cosf(fRotation) * fDistFromCenterOutter;
			const float fYInner = -sinf(fRotation) * fDistFromCenterInner;
			const float fYOutter = -sinf(fRotation) * fDistFromCenterOutter;

			v[i*2+0].p = D3DXVECTOR3( fXInner, fYInner, 0 );
			v[i*2+1].p = D3DXVECTOR3( fXOutter, fYOutter,	0 );
			v[i*2+0].color = PlayerToColor( (PlayerNumber)p );
			v[i*2+1].color = v[i*2+0].color;
		}

		DISPLAY->AddStrip( v, 10 );
	}
}

void GrooveRadar::GrooveRadarValueMap::TweenOnScreen()
{
	SetZoom( 0.5f );
	SetRotation( D3DX_PI*4 );
	BeginTweening( 0.6f );
	SetTweenZoom( 1 );
	SetTweenRotationZ( 0 );
}

void GrooveRadar::GrooveRadarValueMap::TweenOffScreen()
{
	BeginTweening( 0.6f );
	SetTweenRotationZ( D3DX_PI*4 );
	SetTweenZoom( 0 );
}
