#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DoingDancingApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void DoingDancingApp::setup()
{
}

void DoingDancingApp::mouseDown( MouseEvent event )
{
}

void DoingDancingApp::update()
{
}

void DoingDancingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( DoingDancingApp, RendererGl )
