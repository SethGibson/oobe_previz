#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"
#include "pxcsensemanager.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class WaterTestApp : public AppNative {
public:
	void prepareSettings(Settings *pSettings);
	void setup();
	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	
	void mouseUp( MouseEvent event );	
	void update();
	void draw();
	void shutdown();

private:
	int mDebugDraw;
	uint16_t *mBufferDepth;
	Surface8u mSurfDepth;
	gl::Texture mTexSeg, mTexRgb, mTexBg;

	Vec2f mSizePixel;
	size_t mIndexFbo;
	gl::Fbo mFbo;
	
	Vec2i mMouse;
	bool mMouseDown, mUpdated;

	gl::GlslProg mShaderRipple, mShaderDisplace;

	params::InterfaceGl mGUI;
	float mDampen, mPower, mSpeed, mCutoff;


	PXCSenseManager *mPXC;
	PXCSizeI32 mDepthSize;
	
	void getDepthSurface();
	void setupShaders();
	void setupFbos();
	void updateFbos();
	void setupGUI();
};

void WaterTestApp::prepareSettings(Settings *pSettings)
{
	pSettings->setWindowSize(640,480);
	pSettings->setFrameRate(30);
}

void WaterTestApp::setup()
{
	setupShaders();
	setupFbos();
	setupGUI();
	mDebugDraw = 0;
	if(PXCSenseManager::CreateInstance(&mPXC)>=PXC_STATUS_NO_ERROR)
	{
		//mPXC->EnableVideoStream(PXCImage::COLOR_FORMAT_RGB24, 640, 480,0);
		mPXC->EnableVideoStream(PXCImage::COLOR_FORMAT_DEPTH, 640, 480, 0);
		if(mPXC->Init()>=PXC_STATUS_NO_ERROR)
		{
			mDepthSize = mPXC->QueryCaptureManager()->QueryImageSizeByType(PXCImage::IMAGE_TYPE_DEPTH);
			mBufferDepth = new uint16_t[mDepthSize.width*mDepthSize.height];

			mSurfDepth = Surface8u(mDepthSize.width, mDepthSize.height, true, SurfaceChannelOrder::RGBA);
			mTexBg = loadImage(loadAsset("bg_img.jpg"));

			mSizePixel = Vec2f::one()/Vec2f(160,120);
		}
	}
	else
		quit();
}

void WaterTestApp::mouseDown( MouseEvent event )
{
	mMouseDown = true;
	mouseDrag( event );
}

void WaterTestApp::mouseDrag( MouseEvent event )
{
	mMouse = event.getPos();
}

void WaterTestApp::mouseUp( MouseEvent event )
{
	mMouseDown = false;
}

void WaterTestApp::update()
{
	mUpdated = false;
	if(mPXC->AcquireFrame(true,0)>=PXC_STATUS_NO_ERROR)
	{
	/*	PXCImage *cImgRgb = mPXC->QueryImageByType(PXCImage::IMAGE_TYPE_COLOR);
		if(cImgRgb)
		{
			PXCImage::ImageData cDataRgb;
			if(cImgRgb->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_RGB24, &cDataRgb)>=PXC_STATUS_NO_ERROR)
			{
				mTexRgb = gl::Texture(cDataRgb.planes[0], GL_BGR, 640,480);
				cImgRgb->ReleaseAccess(&cDataRgb);
			}
		}*/

		PXCImage *cImgDepth = mPXC->QueryImageByType(PXCImage::IMAGE_TYPE_DEPTH);
		if(cImgDepth)
		{
			PXCImage::ImageData cDataDepth;
			if(cImgDepth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_DEPTH, &cDataDepth)>=PXC_STATUS_NO_ERROR)
			{
				mBufferDepth = (uint16_t *)cDataDepth.planes[0];
				getDepthSurface();
				updateFbos();
				mUpdated = true;
				cImgDepth->ReleaseAccess(&cDataDepth);
			}

			if(cImgDepth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_RGB32, &cDataDepth)>=PXC_STATUS_NO_ERROR)
			{
				mTexRgb = gl::Texture(cDataDepth.planes[0], GL_RGBA, 640, 480);
				cImgDepth->ReleaseAccess(&cDataDepth);
			}
		}
		mPXC->ReleaseFrame();
	}
}

void WaterTestApp::updateFbos()
{
	gl::enable( GL_TEXTURE_2D );
	gl::color( Colorf::white() );

	size_t pong = ( mIndexFbo + 1 ) % 2;
	mFbo.bindFramebuffer();
	glDrawBuffer( GL_COLOR_ATTACHMENT0 + pong );

	gl::setViewport( mFbo.getBounds() );
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::clear();

	mFbo.bindTexture( 0, mIndexFbo );
	mShaderRipple.bind();
	mShaderRipple.uniform("dampen", mDampen);
	mShaderRipple.uniform("power", mPower);
	mShaderRipple.uniform("speed", mSpeed);
	mShaderRipple.uniform( "pixel", mSizePixel );
	mShaderRipple.uniform( "texBuffer", 0 ); 
	gl::drawSolidRect(Rectf(0,0,640,480));
	mShaderRipple.unbind();
	mFbo.unbindTexture();
	gl::disable( GL_TEXTURE_2D );

	gl::enableAlphaBlending();
	gl::draw(gl::Texture(mSurfDepth),Rectf(0,0,getWindowWidth(),getWindowHeight()));
	gl::disableAlphaBlending();

	mFbo.unbindFramebuffer();
	mIndexFbo = pong;
}

void WaterTestApp::draw()
{
	gl::clear(Color::black());
	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());

	if(mTexRgb&&mUpdated)
	{
		mFbo.bindTexture(0,mIndexFbo);
		gl::enable(GL_TEXTURE_2D);
		mTexRgb.bind(1);

		mShaderDisplace.bind();
		mShaderDisplace.uniform("texBuffer",0);
		mShaderDisplace.uniform("pixel",mSizePixel);
		mShaderDisplace.uniform("texRefract",1);

		gl::drawSolidRect(Rectf(0,0,getWindowWidth(),getWindowHeight()));
		mTexRgb.unbind();
		gl::disable(GL_TEXTURE_2D);
		mShaderDisplace.unbind();
	}

	mGUI.draw();
	/*
	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());
	gl::clear(Color::black());
	gl::color(Color::white());
	if(mTexRgb&&mUpdated)
	{
		gl::draw(mTexRgb, Rectf(Vec2f::zero(),getWindowCenter()));
		gl::draw(gl::Texture(mSurfDepth), Rectf(getWindowCenter(), getWindowSize()));
	}*/
}

void WaterTestApp::getDepthSurface()
{
	Surface8u::Iter iter = mSurfDepth.getIter(mSurfDepth.getBounds());
	while(iter.line())
	{
		while(iter.pixel())
		{
			iter.r() = 0;
			iter.g() = 0;
			iter.b() = 0;
			iter.a() = 0;
			uint16_t cVal = mBufferDepth[iter.y()*mDepthSize.width+iter.x()];
			if(cVal>0&&cVal<mCutoff)
			{
				float cColor = lmap<float>((float)cVal,0,mCutoff,0,255);
				iter.r() = (uint8_t)cColor;
				iter.a() = (uint8_t)cColor;
			}
		}
	}
}

void WaterTestApp::setupShaders()
{
	try
	{
		mShaderRipple = gl::GlslProg(loadAsset("shaders/passthruVert.glsl"), loadAsset("shaders/rippleFrag.glsl"));
	}
	catch(gl::GlslProgCompileExc ex)
	{
		console() << "Ripple Shader Error: " << ex.what() << endl;
	}
	try
	{
		mShaderDisplace = gl::GlslProg(loadAsset("shaders/passthruVert.glsl"), loadAsset("shaders/displaceFrag.glsl"));
	}
	catch(gl::GlslProgCompileExc ex)
	{
		console() << "Displace Shader Error: " << ex.what() << endl;
	}
}

void WaterTestApp::setupFbos()
{
	gl::Fbo::Format format;
	format.enableColorBuffer( true, 2 );
	format.setMagFilter( GL_NEAREST );
	format.setMinFilter( GL_NEAREST );
	format.setColorInternalFormat( GL_RGBA32F_ARB );

	// Create a frame buffer object with two attachments ping pong
	mIndexFbo	= 0;
	mFbo		= gl::Fbo( 640, 480, format );
	mFbo.bindFramebuffer();
	const GLenum buffers[ 2 ] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers( 2, buffers );
	gl::setViewport( mFbo.getBounds() );
	gl::clear();
	mFbo.unbindFramebuffer();
	for ( size_t i = 0; i < 2; ++i ) {
		mFbo.getTexture( i ).setWrap( GL_REPEAT, GL_REPEAT );
	}
}
void WaterTestApp::setupGUI()
{
	mCutoff = 200;
	mDampen = 0.9f;
	mPower = 1.0f;
	mSpeed = 5.0f;
	mGUI = params::InterfaceGl("Tweaks", Vec2i(200,100));
	mGUI.addParam("Depth", &mCutoff);
	mGUI.addParam("Dampen", &mDampen);
	mGUI.addParam("Speed", &mSpeed);
	mGUI.addParam("Power", &mPower);
}

void WaterTestApp::shutdown()
{
	if(mPXC)
		mPXC->Close();
}

CINDER_APP_NATIVE( WaterTestApp, RendererGl )
