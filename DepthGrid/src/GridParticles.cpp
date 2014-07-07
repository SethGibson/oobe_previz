#include "GridParticles.h"

GridParticle::GridParticle()
{
	IsAlive=true;
}

GridParticle::GridParticle(float pLife, float pSize, Vec2f pPos, Vec2f pVel):mLife(pLife),mSize(pSize),mPos(pPos),mVel(pVel)
{
	IsAlive = true;
	mAge = 0;
}

GridParticle::~GridParticle()
{
}

void GridParticle::step()
{
	if(mAge==mLife)
	{
		IsAlive = false;
		return;
	}
	++mAge;
	mPos+=mVel;
}

void GridParticle::display()
{
	float cAlpha = 1.0f-lmap<float>(mAge,0.0f,mLife,0.0f,1.0f);
	float cSize = lerp<float>(0,mSize,cAlpha)*0.5f;
	gl::color(ColorA(0,0,0,cAlpha));
	gl::drawSolidRect(Rectf(mPos.x-cSize,mPos.y-cSize,mPos.x+cSize, mPos.y+cSize));
	gl::color(ColorA(0.9f,0.9f,0.9f,cAlpha));
	gl::drawStrokedRect(Rectf(mPos.x-cSize,mPos.y-cSize,mPos.x+cSize, mPos.y+cSize));
}

GridParticleMgr::GridParticleMgr()
{
}

GridParticleMgr::~GridParticleMgr()
{
}

void GridParticleMgr::step()
{
	for(auto pit=mParticles.begin();pit!=mParticles.end();)
	{
		if(!pit->IsAlive)
			mParticles.erase(pit);
		else
		{
			pit->step();
			pit++;
		}
	}
}

void GridParticleMgr::display()
{
	gl::enableAlphaBlending();
	for(auto p : mParticles )
		p.display();
	gl::disableAlphaBlending();
}

void GridParticleMgr::add(float pSize, Vec2f pPos)
{
	float cLife = randFloat(20,75);
	Vec2f cVel = Vec2f(randFloat(-3,-8), randFloat(-3,-8));
	mParticles.push_back(GridParticle(cLife,pSize,pPos,cVel));
}