#ifndef __GRIDPARTICLES_H__
#define __GRIDPARTICLES_H__
#include <vector>
#include "cinder/CinderMath.h"
#include "cinder/Color.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Vector.h"

using namespace std;
using namespace ci;

class GridParticle
{
public:
	GridParticle();
	GridParticle(float pLife, float pSize, Vec2f pPos, Vec2f pVel);
	~GridParticle();

	void step();
	void display();

	bool IsAlive;

private:
	float mAge, mLife, mSize;
	Vec2f mPos, mVel;
};

class GridParticleMgr
{
public:
	GridParticleMgr();
	~GridParticleMgr();

	void step();
	void display();
	void add(float pSize, Vec2f pPos);

private:
	vector<GridParticle> mParticles;
};
#endif