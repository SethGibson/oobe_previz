#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Rand.h"
#include "CinderOpenCV.h"
#include "pxcsensemanager.h"
#include "GridParticles.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DepthGridApp : public AppNative {
public:
	void prepareSettings(Settings *pSettings);
	void setup();
	void update();
	void draw();
	void shutdown();

private:
	void setupShaders();
	void updateInputs();
	Vec2f getCentroid(vector<cv::Point> &pContour);
	
	bool mSpawn;
	float mGridSize;
	uint8_t *mDepthBuffer;
	Vec2f mCenter;
	vector<Vec2f> mSpawnPoints;
	vector<vector<cv::Point>> mContours;

	Surface8u mSurf;
	gl::Texture mTex;
	gl::GlslProg mShaderGrid;

	PXCSenseManager *mPXC;
	PXCSizeI32 mDepthSize;
	
	GridParticleMgr mParticles;

};

void DepthGridApp::prepareSettings(Settings *pSettings)
{
	pSettings->setWindowSize(640,480);
	pSettings->setFrameRate(30);
}

void DepthGridApp::setup()
{
	mSpawn = false;
	mGridSize = 7.0f;
	mCenter = Vec2f::zero();

	if(PXCSenseManager::CreateInstance(&mPXC)>=PXC_STATUS_NO_ERROR)
	{
		mPXC->EnableVideoStream(PXCImage::COLOR_FORMAT_DEPTH, 640, 480, 0);
		if(mPXC->Init()>=PXC_STATUS_NO_ERROR)
		{
			mDepthSize = mPXC->QueryCaptureManager()->QueryImageSizeByType(PXCImage::IMAGE_TYPE_DEPTH);
			mSurf = Surface8u((int32_t)mDepthSize.width, (int32_t)mDepthSize.height, true, SurfaceChannelOrder::RGBA);
			mDepthBuffer = new uint8_t[mDepthSize.width*mDepthSize.height*4];
		}
		setupShaders();
	}
	else
		quit();
}

void DepthGridApp::update()
{
	if(mPXC->AcquireFrame(true,0)>=PXC_STATUS_NO_ERROR)
	{
		PXCImage *cImgDepth = mPXC->QueryImageByType(PXCImage::IMAGE_TYPE_DEPTH);
		if(cImgDepth)
		{
			PXCImage::ImageData cDataDepth;
			if(cImgDepth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_RGB32, &cDataDepth)>=PXC_STATUS_NO_ERROR)
			{
				memcpy(mDepthBuffer, (uint8_t *)cDataDepth.planes[0], (size_t)(cDataDepth.pitches[0]*mDepthSize.height));
				cImgDepth->ReleaseAccess(&cDataDepth);
			}
		}
		mPXC->ReleaseFrame();
	}
	mSurf = Surface8u(mDepthBuffer, (int32_t)mDepthSize.width, (int32_t)mDepthSize.height, (int32_t)(mDepthSize.width*4), SurfaceChannelOrder::RGBA);
	mTex = gl::Texture(mSurf);

	updateInputs();
}

void DepthGridApp::updateInputs()
{
	mContours.clear();
	cv::findContours(toOcv(Channel8u(mSurf)), mContours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

	if(mContours.size()>0) // avoid divide by zero
	{
		float cNumPts = mContours.size();
		Vec2f cCenter;
		for(auto cit = mContours.begin();cit!=mContours.end();++cit)
		{
			cCenter+=Vec2f(getCentroid(*cit));

			if(getElapsedFrames() % (30+randInt(0,10))==0)
			{
				mSpawn = true;
				vector<cv::Point> cContour = *cit;
				Rectf cBound = fromOcv(cv::boundingRect(cContour));
				if(cBound.calcArea()>100)
				{
					//store the upper part of the contour's bounds
					//to give the effect of particles only spawning
					//from the upper portion of head, body, etc.
					Rectf cSearchRect = Rectf(cBound);
					cSearchRect.y2 = cBound.y2*0.1f;
					
					vector<int> cHull;
					vector<cv::Vec4i> cDefects;
					cv::convexHull(cContour, cHull);
					if(cContour.size()>3&&cHull.size()>0) //since OpenCV doesn't handle this for us...like it should.
					{
						cv::convexityDefects(cContour, cHull, cDefects);

						//find potential spawn points
						for(auto dit=cDefects.begin();dit!=cDefects.end();++dit)
						{
							cv::Vec4i cDefect = *dit;
							float cDepth = cDefect[3]/256.0f;
							if(cDepth>=mGridSize)
							{
								Vec2f cPoint = fromOcv(cContour.at(cDefect[2]));
								if(cSearchRect.contains(cPoint));
									mSpawnPoints.push_back(cPoint);
							}
						}
					}
				}
			}
		}
		mCenter = Vec2f(cCenter/cNumPts);
	}

	if(mSpawn)
	{
		for(auto it=mSpawnPoints.begin();it!=mSpawnPoints.end();++it)
			mParticles.add(randFloat(2,mGridSize*1.5f), *it-Vec2f(mGridSize*2, mGridSize*2));
		mSpawn = false;
		mSpawnPoints.clear();
	}

	mParticles.step();
}

void DepthGridApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
	gl::color(Color::white());
	if(mTex)
	{
		gl::enable(GL_TEXTURE_2D);
		mShaderGrid.bind();
		mTex.bind(0);
		mShaderGrid.uniform("gTexDepth", 0);
		mShaderGrid.uniform("gOffset", mCenter);
		mShaderGrid.uniform("gGridSize", mGridSize);
		gl::drawSolidRect(getWindowBounds());
		mTex.unbind();
		mShaderGrid.unbind();
		gl::disable(GL_TEXTURE_2D);
	}
	mParticles.display();
}

void DepthGridApp::setupShaders()
{
	try
	{
		mShaderGrid = gl::GlslProg(loadAsset("passthru.vert"), loadAsset("gridblend.frag"));
	}
	catch(const gl::GlslProgCompileExc &e)
	{
		console() << "Shader Error: " << e.what() << endl;
		quit();
	}
}

Vec2f DepthGridApp::getCentroid(vector<cv::Point> &pContour)
{
	float cNumPts = pContour.size();
	Vec2f cCentroid;
	for(auto pit=pContour.begin();pit!=pContour.end();++pit)
	{
		cCentroid+=fromOcv(*pit);
	}

	return Vec2f(cCentroid/cNumPts);
}

void DepthGridApp::shutdown()
{
	if(mPXC)
		mPXC->Close();
}

CINDER_APP_NATIVE( DepthGridApp, RendererGl )
