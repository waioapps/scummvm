/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */


#ifndef ACTOR_H
#define ACTOR_H

#include <string.h>
#include "scummsys.h"

class Scumm;

enum MoveFlags {
	MF_NEW_LEG = 1,
	MF_IN_LEG = 2,
	MF_TURN = 4,
	MF_LAST_LEG = 8
};

struct ActorWalkData {
	int16 destx,desty;			// Final destination
	byte destbox;
	int16 destdir;
	byte curbox;
	int16 x,y;					// Current position
	int16 newx,newy;			// Next position on our way to the destination
	int32 XYFactor, YXFactor;
	uint16 xfrac,yfrac;
	int point3x, point3y;
};

struct CostumeData {
	byte active[16];
	uint16 animCounter1;
	byte animCounter2;
	uint16 stopped;
	uint16 curpos[16];
	uint16 start[16];
	uint16 end[16];
	uint16 frame[16];
	
	void reset()
	{
		stopped = 0;
		for (int i = 0; i < 16; i++) {
			active[i] = 0;
			curpos[i] = start[i] = end[i] = frame[i] = 0xFFFF;
	}
}
};

class Actor {

//protected:
public:
	int x, y, top, bottom;
	int elevation;
	uint width;
	byte number;
	uint16 facing;
	uint16 costume;
	byte room;
	byte talkColor;
	byte scalex,scaley;
	byte charset;
	int16 newDirection;
	byte moving;
	byte ignoreBoxes;
	byte forceClip;
	byte initFrame, walkFrame, standFrame, talkFrame1, talkFrame2;
	bool needRedraw, needBgReset, costumeNeedsInit, visible;
	byte shadow_mode;
	bool flip;
	uint speedx,speedy;
	byte frame;
	byte walkbox;
	byte mask;
	byte animProgress, animSpeed;
	int16 new_1,new_2;
	uint16 talk_script, walk_script;
	byte new_3;
	int8 layer;
	ActorWalkData walkdata;
	int16 animVariable[16];
	uint16 sound[8];
	CostumeData cost;
	byte palette[64];

protected:
	Scumm	*_vm;

public:

	// Constructor, sets all data to 0
	Actor() { memset(this, 0, sizeof(Actor)); }
    void initActorClass(Scumm *scumm) {_vm = scumm;}
//protected:
	void hideActor();
	void showActor();

	void initActor(int mode);
	void setActorWalkSpeed(uint newSpeedX, uint newSpeedY);
	int calcMovementFactor(int newx, int newy);
	int actorWalkStep();
	int remapDirection(int dir);
	void setupActorScale();
	void stopActorMoving();
	void startWalkAnim(int cmd, int angle);
	void startAnimActor(int frame);
	void setActorBox(int box);

	int updateActorDirection();
	void setActorDirection(int direction);

	void adjustActorPos();	
	void turnToDirection(int newdir);
	void walkActor();
	void drawActorCostume();
	void actorAnimate();
	void setActorCostume(int c);
	byte *getActorName();
	void startWalkActor(int x, int y, int dir);
	
	void remapActor(int b, int c, int d, int e);
	void walkActorOld();
	
	bool isInCurrentRoom()					{ return room == _vm->_currentRoom; }

	int getAnimVar(byte var)				{ return animVariable[var]; }
	void setAnimVar(byte var, int value)	{ animVariable[var] = value; }

};

#endif
