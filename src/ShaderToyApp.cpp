//CINDER
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/qtime/AvfWriter.h"
#include "cinder/gl/ShaderPreprocessor.h"

//BOOST
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"

//BLOCKS
#include "UI.h"
#include "Tiler.h"
#include "LiveAsset.h"
#include "GlslParams.h"
#include "Paths.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace reza::live;
using namespace reza::glsl;
using namespace reza::ui;
using namespace reza::tiler;
using namespace reza::paths;

class ShaderToyApp : public App {
public:
    //PREPARE SETTINGS
    static void prepareSettings( Settings *settings );
    
    //SETUP
    void setup() override;
    
    //CLEANUP
    void cleanup() override;
    
    //FILE SYSTEM
    void save( const fs::path& path );
    void load( const fs::path& path );
    
    //OUTPUT
    void setupOutput();
    void updateOutput();
    void drawOutput();
    void _drawOutput();
    void _drawOutput( const vec2 &ul, const vec2 &ur, const vec2 &lr, const vec2 &ll );
    void keyDownOutput( KeyEvent event );
    void mouseDownOutput( MouseEvent event );
    void mouseDragOutput( MouseEvent event );
    
    ci::app::WindowRef mOutputWindowRef;
    vec2 mMouse = vec2( 0.0 );
    vec2 mMousePrev = vec2( 0.0 );
    vec2 mMouseClick = vec2( 0.0 );
    ivec2 mOutputWindowOrigin = ivec2( 0 );
    ivec2 mOutputWindowSize = ivec2( 1920, 1080 );
    
    //BACKGROUND
    ColorA mBgColor = ColorA::white();
    
    //COLOR PALETTE
    Surface32fRef mPaletteSurfRef = nullptr;
    gl::Texture2dRef mPaletteTexRef = nullptr;
    void setupPalettes();
    
    //SAVE IMAGE
    bool mSaveImage = false;
    ci::fs::path mSaveImagePath;
    std::string mSaveImageName;
    std::string mSaveImageExtension;
    int mSaveImageFrame = -1;
    int mSaveImageSizeMultiplier = 4;
    
    void saveImage();
    void saveImageAs();
    void saveImage( const ci::fs::path& path, const std::string &filename, const std::string &extension );
    
    //MOVIE EXPORTER
    TilerRef mSequenceTiler;
    fs::path mSequenceTilerPath;
    qtime::MovieWriterRef mMovieExporter;
    int mMovieExporterTotalFrames = 0;
    float mMovieExporterProgress = 0.0f;
    bool mMovieExporterRecording = false;
    bool mMovieExporterSaveMovie = true;
    bool mMovieExporterSaveFrames = false;
    int mMovieExporterStartFrame = 0;
    void setupRecorder();
    void recordOutput();
    
    //BATCH & GLSL
    bool mSetupBatch = true;
    gl::BatchRef mBatchRef = nullptr;
    gl::GlslProgRef mGlslRef = nullptr;
    LiveAssetRef mGlslLiveRef = nullptr;
    GlslParams mGlsParams;
    
    void setupBatch();
    void drawBatch();
    void setupGlsl();
    
    //UI
    void setupUIs();
    void loadUIs( const fs::path &path );
    void saveUIs( const fs::path &path );
    void loadUI( const fs::path &path, const string &uiName );
    void saveUI( const fs::path &path, const string &uiName );
    void killUI( WindowCanvasRef ui );
    
    WindowCanvasRef createUI( const std::string& name );
    void closeUI( const string& name );
    void spawnUI( const string& name );
    
    WindowCanvasRef addUI( WindowCanvasRef ui );
    void addShaderParamsUI( WindowCanvasRef &ui, GlslParams& glslParams, gl::GlslProgRef& glslProgRef );
    void right( WindowCanvasRef& ui );
    void down( WindowCanvasRef& ui );
    map<string, WindowCanvasRef> mUIMap;
    
    //UIS
    WindowCanvasRef setupUI( WindowCanvasRef ui );
    WindowCanvasRef setupShaderUI( WindowCanvasRef ui );
    WindowCanvasRef setupExporterUI( WindowCanvasRef ui );
    
    void loadGlsl( const fs::path &path );
    void saveGlsl( const fs::path &path );
    map<string, fs::path> mGlslPathMap;
    
    //CAMERA
    float mDoubleClickThreshold = 0.2;
    float mLastClick = -1;
    
    vec2 mTexcoordOffset = vec2( 0.0f );
    float mTexcoordScale = 0.0;
    
    bool firstTime = true;
};

//------------------------------------------------------------------------------
#pragma mark - PREPARE SETTINGS
//------------------------------------------------------------------------------

void ShaderToyApp::prepareSettings( App::Settings *settings )
{
    settings->setWindowSize( 1280, 720 );
    settings->setFrameRate( 60.0f );
    settings->setHighDensityDisplayEnabled();
}

//------------------------------------------------------------------------------
#pragma mark - SETUP
//------------------------------------------------------------------------------

void ShaderToyApp::setup()
{
    {
        Timer mTime;
        mTime.start();
        createAssetDirectories();
        mTime.stop();
        cout << "createAssetDirectories: " << mTime.getSeconds() << endl;
    }
    {
        Timer mTime;
        mTime.start();
        setupPalettes();
        mTime.stop();
        cout << "setupPalettes: " << mTime.getSeconds() << endl;
    }
    {
        Timer mTime;
        mTime.start();
        setupGlsl();
        mTime.stop();
        cout << "setupGlsl: " << mTime.getSeconds() << endl;
    }
    {
        Timer mTime;
        mTime.start();
        setupOutput();
        mTime.stop();
        cout << "setupOutput: " << mTime.getSeconds() << endl;
    }
    {
        Timer mTime;
        mTime.start();
        setupUIs();
        mTime.stop();
        cout << "setupUIs: " << mTime.getSeconds() << endl;
    }
    {
        Timer mTime;
        mTime.start();
        load( getWorkingPath() );
        mTime.stop();
        cout << "load: " << mTime.getSeconds() << endl;
    }
}

//------------------------------------------------------------------------------
#pragma mark - CLEANUP
//------------------------------------------------------------------------------

void ShaderToyApp::cleanup()
{
    save( getWorkingPath() );
}

//------------------------------------------------------------------------------
#pragma mark - FILESYSTEM
//------------------------------------------------------------------------------

void ShaderToyApp::save( const fs::path& path )
{
    createDirectory( path );
    saveGlsl( path );
    saveUIs( path );
}

void ShaderToyApp::load( const fs::path& path )
{
    loadUIs( path );
    if( !firstTime ) {
        loadGlsl( path );
        firstTime = false; 
    }
}

//------------------------------------------------------------------------------
#pragma mark - OUTPUT
//------------------------------------------------------------------------------

void ShaderToyApp::setupOutput()
{
    mOutputWindowRef = getWindow();
    mOutputWindowRef->getSignalClose().connect( [ this ] { quit(); } );
    mOutputWindowRef->getSignalDraw().connect( [ this ] { updateOutput(); drawOutput(); recordOutput(); } );
    mOutputWindowRef->getSignalResize().connect( [ this ] { mOutputWindowSize = mOutputWindowRef->getSize(); mSetupBatch = true; } );
    mOutputWindowRef->getSignalMove().connect( [ this ] { mOutputWindowOrigin = mOutputWindowRef->getPos(); } );
    mOutputWindowRef->getSignalKeyDown().connect( [ this ] ( KeyEvent event ) { keyDownOutput( event ); } );
    mOutputWindowRef->getSignalMouseDown().connect( [ this ] ( MouseEvent event ) {
        mMousePrev = mMouseClick = mMouse = vec2( event.getPos() );
        if( ( ( getElapsedSeconds() - mLastClick ) < mDoubleClickThreshold ) ) {
            mTexcoordOffset = vec2( 0.0f );
            mTexcoordScale = 0.0;
            mSetupBatch = true;
        }
        mLastClick = getElapsedSeconds();
    } );
    mOutputWindowRef->getSignalMouseDrag().connect( [ this ] ( MouseEvent event ) {
        mMouse = event.getPos();
        if( event.isMetaDown() ) {
            vec2 delta = ( mMousePrev - mMouse ) / ( vec2( mOutputWindowRef->getSize() ) * toPixels( 1.0f + mTexcoordScale ) );
            delta.y *= -1.0;
            mTexcoordOffset += delta;
            mSetupBatch = true;
        }
        mMousePrev = mMouse;
    } );
    mOutputWindowRef->getSignalMouseUp().connect( [ this ] ( MouseEvent event ) { mMouse = vec2( 0.0 ); } );
    mOutputWindowRef->getSignalMouseWheel().connect( [ this ] ( MouseEvent event ) {
        if( event.isMetaDown() ) {
            mTexcoordScale += event.getWheelIncrement() * 0.001;
            mSetupBatch = true;
        }
    } );
}

void ShaderToyApp::updateOutput()
{
    mOutputWindowRef->setTitle( to_string( (int) getAverageFps() ) + " FPS" );
    
    if( mSetupBatch ) {
        setupBatch();
        mSetupBatch = false;
    }
    
    if( mSaveImage && ( getElapsedFrames() - mSaveImageFrame ) > 2 ) {
        saveImage( mSaveImagePath, mSaveImageName, mSaveImageExtension );
        mSaveImage = false;
    }
    
}

void ShaderToyApp::drawOutput()
{
    gl::clear( mBgColor );
    vec2 size = mOutputWindowRef->getSize();
    gl::setMatricesWindow( size );
    auto t = boost::posix_time::second_clock::local_time();
    auto date = t.date();
    mGlslRef->uniform( "iResolution", vec3( size.x, size.y, 0.0 ) );
    mGlslRef->uniform( "iGlobalTime", float( getElapsedSeconds() ) );
    mGlslRef->uniform( "iMouse", vec4( mMouse.x, size.y - mMouse.y, mMouseClick.x, size.y - mMouseClick.y ) );
    mGlslRef->uniform( "iDate", vec4( date.year(), date.month(), date.day_number(), t.time_of_day().seconds() ) );
    mGlslRef->uniform( "iPalettes", 0 );
    mPaletteTexRef->bind( 0 );
    mGlsParams.applyUniforms( mGlslRef );
    _drawOutput();
}

void ShaderToyApp::_drawOutput()
{
    gl::ScopedBlendAlpha scpAlp;
    drawBatch();
}

void ShaderToyApp::_drawOutput( const vec2 &ul, const vec2 &ur, const vec2 &lr, const vec2 &ll )
{
    gl::ScopedBlendAlpha scpAlp;
    vec2 size = mOutputWindowRef->getSize();
    auto batch = gl::Batch::create( geom::Rect( Rectf( 0.0f, 0.0f, size.x, size.y ) ).texCoords( ul, ur, lr, ll ), mGlslRef );
    batch->draw();
}

void ShaderToyApp::keyDownOutput( KeyEvent event )
{
    if( event.isMetaDown() ) {
        switch ( event.getCode() ) {
            case KeyEvent::KEY_a: { spawnUI( "shadertoy" ); } break;
            case KeyEvent::KEY_p: { spawnUI( "params" ); } break;
            case KeyEvent::KEY_e: { spawnUI( "exporter" ); } break;
            case KeyEvent::KEY_f: {
                mOutputWindowRef->setFullScreen( !mOutputWindowRef->isFullScreen() );
            }
                break;
        }
    }
}

//------------------------------------------------------------------------------
#pragma mark - BATCH
//------------------------------------------------------------------------------

void ShaderToyApp::setupBatch()
{
    vec2 size = mOutputWindowRef->getSize();
    vector<vec2> texcoords = { vec2( 0.0, 1.0 ), vec2( 1.0, 1.0 ), vec2( 1.0, 0.0 ), vec2( 0.0, 0.0 ) };
    for( auto& it : texcoords ) {
        it += normalize( vec2( 0.5 ) - it ) * mTexcoordScale;
        it += mTexcoordOffset;
    }
    auto geo = geom::Rect( Rectf( 0.0f, 0.0f, size.x, size.y ) );
    geo.texCoords( texcoords[0], texcoords[1], texcoords[2], texcoords[3] );
    mBatchRef = gl::Batch::create( geo, mGlslRef );
}

void ShaderToyApp::drawBatch()
{
    mBatchRef->draw();
}

//------------------------------------------------------------------------------
#pragma mark - UI
//------------------------------------------------------------------------------

void ShaderToyApp::setupUIs()
{
    addUI( setupUI( createUI( "shadertoy" ) ) );
    addUI( setupShaderUI( createUI( "params") ) );
    addUI( setupExporterUI( createUI( "exporter" ) ) );
}

WindowCanvasRef ShaderToyApp::setupUI( WindowCanvasRef ui )
{
    auto mvCb = [ this ] ( int value ) { mOutputWindowRef->setPos( mOutputWindowOrigin ); };
    auto szCb = [ this ] ( int value ) { mOutputWindowRef->setSize( mOutputWindowSize ); };
    auto dfmt = Dialeri::Format().label( false );
    ui->addDialeri( "PX", &mOutputWindowOrigin.x, 0, getDisplay()->getWidth(), dfmt )->setCallback( mvCb );
    right( ui );
    ui->addDialeri( "PY", &mOutputWindowOrigin.y, 0, getDisplay()->getHeight(), dfmt )->setCallback( mvCb );
    ui->addDialeri( "SX", &mOutputWindowSize.x, 0, 10000, dfmt )->setCallback( szCb );
    ui->addDialeri( "SY", &mOutputWindowSize.y, 0, 10000, dfmt )->setCallback( szCb );
    ui->addToggle( "BORDER", false, Toggle::Format().label(false) )->setCallback( [ this ] ( bool value ) {
        mOutputWindowRef->setBorderless( value );        
    } );
    down( ui );
    
    ui->addSpacer();
    ui->addButton( "SAVE AS", false )->setCallback([ this ]( bool value ) {
        if( value ) {
            fs::path pth = getSaveFilePath( getPresetsPath() );
            string folderName = pth.filename().string();
            if( folderName.length() ) {
                save( pth );
                save( getWorkingPath() );
            }
        }
    } )->bindToKey( KeyEvent::KEY_s, KeyEvent::META_DOWN );
    right( ui );
    ui->addButton( "LOAD", false )->setCallback( [ this ]( bool value ) {
        if( value ) {
            auto pth = getFolderPath( getPresetsPath() );
            if( !pth.empty() ) {
                load( pth );
            }
        }
    });
    down( ui );
    ui->addSpacer();
    ui->addDialerf( "MX", &mMouse.x, 0.0, 2000 );
    right( ui );
    ui->addDialerf( "MY", &mMouse.y, 0.0, 2000 );
    down( ui );
    ui->addDialerf( "CX", &mMouseClick.x, 0.0, 2000 );
    right( ui );
    ui->addDialerf( "CY", &mMouseClick.y, 0.0, 2000 );
    down( ui );
    
    auto bcb = [ this ] ( float value ) { mSetupBatch = true; };
    ui->addDialerf( "TX", &mTexcoordOffset.x, -2.0, 2.0 )->setCallback( bcb );
    right( ui );
    ui->addDialerf( "TY", &mTexcoordOffset.y, -2.0, 2.0 )->setCallback( bcb );
    ui->addDialerf( "TS", &mTexcoordScale, -2.0, 2.0 )->setCallback( bcb );
    down( ui );
    return ui;
}

WindowCanvasRef ShaderToyApp::setupShaderUI( WindowCanvasRef ui )
{
    ui->addColorPicker( "BACKGROUND COLOR", &mBgColor );
    addShaderParamsUI( ui, mGlsParams, mGlslRef );
    return ui;
}

WindowCanvasRef ShaderToyApp::setupExporterUI( WindowCanvasRef ui )
{
    ui->addButton( "SAVE IMAGE", false )->setCallback( [ this ] ( bool value ) { if( value ) { saveImage(); } } )->bindToKey( KeyEvent::KEY_r, KeyEvent::META_DOWN );
    right( ui );
    ui->addButton( "SAVE IMAGE AS", false )->setCallback( [ this ] ( bool value ) { if( value ) { saveImageAs(); }
    } );
    down( ui );
    ui->addDialeri( "OUTPUT IMAGE SCALE", &mSaveImageSizeMultiplier, 1, 20 );
    
    ui->addSpacer();
    ui->addButton( "RENDER", false )->setCallback( [ this ] ( bool value ) {
        if( value && ( mMovieExporterSaveMovie || mMovieExporterSaveFrames ) ) {
            mMovieExporterRecording = true;
            setupRecorder();
        }
    } );
    right( ui );
    ui->addToggle( "MOV", &mMovieExporterSaveMovie );
    ui->addToggle( "PNG", &mMovieExporterSaveFrames );
    ui->addDialeri( "FRAMES", &mMovieExporterTotalFrames, 0, 99999, Dialeri::Format().label( false ) );
    down( ui );
    ui->addSliderf( "PROGRESS", &mMovieExporterProgress, 0.0, 1.0, Sliderf::Format().label( false ) );
    return ui;
}

WindowCanvasRef ShaderToyApp::createUI( const string& name )
{
    WindowCanvasRef ui = WindowCanvas::create( name );
    mUIMap[ ui->getName() ] = ui;
    return ui;
}

void ShaderToyApp::closeUI( const string& name )
{
    mUIMap[ name ]->close();
}

void ShaderToyApp::spawnUI( const string& name )
{
    mUIMap[ name ]->spawn();
}

void ShaderToyApp::loadUI( const fs::path &path, const string &uiName )
{
    auto it = mUIMap.find( uiName );
    if( it != mUIMap.end() ) {
        auto ui = it->second;
        fs::path uiPath = path;
        uiPath += fs::path( "/" + uiName + ".json" );
        ui->load( uiPath );
    }
}

void ShaderToyApp::saveUI( const fs::path &path, const string &uiName )
{
    auto it = mUIMap.find( uiName );
    if( it != mUIMap.end() ) {
        auto ui = it->second;
        if( createDirectory( path ) ) {
            fs::path uiPath = path;
            uiPath += fs::path( "/" + uiName + ".json" );
            ui->save( uiPath );
        }
    }
}

void ShaderToyApp::killUI( WindowCanvasRef ui )
{
    string name = ui->getName();
    ui->close();
    mUIMap.erase( mUIMap.find( name ) );
}

void ShaderToyApp::loadUIs( const fs::path &path )
{
    for( auto& it : mUIMap ) {
        loadUI( path, it.first );
    }
}

void ShaderToyApp::saveUIs( const fs::path &path )
{
    for( auto& it : mUIMap ) {
        saveUI( path, it.first );
    }
}

WindowCanvasRef ShaderToyApp::addUI( WindowCanvasRef ui )
{
    ui->autoSizeToFitSubviews();
    return ui;
}

void ShaderToyApp::right( WindowCanvasRef& ui )
{
    ui->setSubViewAlignment( Alignment::NONE );
    ui->setSubViewDirection( Direction::EAST );
}

void ShaderToyApp::down( WindowCanvasRef& ui )
{
    ui->setSubViewAlignment( Alignment::LEFT );
    ui->setSubViewDirection( Direction::SOUTH );
}

void ShaderToyApp::addShaderParamsUI( WindowCanvasRef &ui, GlslParams& glslParams, gl::GlslProgRef& glslProgRef )
{
    /*
     uniform vec3 axis0;     //multi:0.0,1.0,0.5         -> multi
     uniform vec2 axis1;     //multi:0.0,1.0,0.5         -> multi
     uniform int iaxis;      //slider:0.0,1.0,0.5        -> slider
     uniform int axis3;      //dialer:0.0,1.0,0.5        -> dialer
     uniform float axis2;    //slider:0.0,1.0,0.5        -> slider
     uniform float axis3;    //dialer:0.0,1.0,0.5        -> dialer
     uniform float legacy;   //ui:0.0,1.0,0.5            -> multi
     uniform vec2 range2;    //range:0.0,1.0,0.2,0.75    -> range
     uniform vec2 pad;       //xypad:-1.0,1.0,0.0        -> pad
     uniform bool state0;    //button:0                  -> button
     uniform bool state1;    //toggle:1                  -> toggle
     */
    
//    multimap<string, string> uiTypeMap  = {
//        { "int" , "slider" },
//        { "int" , "dialer" },
//        
//        { "float" , "ui" },
//        { "float" , "slider" },
//        { "float" , "dialer" },
//        
//        { "vec2" , "pad" },
//        { "vec2" , "range" },
//        { "vec2" , "ui" },
//        
//        { "vec3" , "ui" },
//        { "vec4" , "ui" },
//        { "vec4" , "color" },
//        
//        { "bool" , "button" },
//        { "bool" , "toggle" }
//    };

    MultiSliderRef ref = nullptr;
    vector<MultiSlider::Data> data;
    
    
    
    auto& order = glslParams.getParamOrder();
    auto& types = glslParams.getTypeMap();

    auto& cp = glslParams.getColorParams();

    auto& bp = glslParams.getBoolParams();
    
    auto& ip = glslParams.getIntParams();
    auto& ir = glslParams.getIntRanges();
    
    auto& fp = glslParams.getFloatParams();
    auto& fr = glslParams.getFloatRanges();
    
    auto& v2p = glslParams.getVec2Params();
    auto& v2r = glslParams.getVec2Ranges();

    auto& v3p = glslParams.getVec3Params();
    auto& v3r = glslParams.getVec3Ranges();

    auto& v4p = glslParams.getVec4Params();
    auto& v4r = glslParams.getVec4Ranges();
    
    for( auto& it : order ) {
        string name = it.second;
        string type = types.at( name ).first;       // float
        string uitype = types.at( name ).second;    // slider
        cout << "INDEX: " << it.first << " " << it.second << endl;
        if( type == "bool" ) {
            bool *ptr = &bp[ name ];
            if( uitype == "button" ) { ui->addButton( name, ptr ); }
            else if( uitype == "toggle" ) { ui->addToggle( name, ptr ); }
        }
        else if( type == "int" ) {
            int* ptr = &ip[ name ];
            int low = ir[ name ].first;
            int high = ir[ name ].second;
            if( uitype == "slider" ) { ui->addSlideri( name, ptr, low, high ); }
            else if( uitype == "dialer" ) { ui->addDialeri( name, ptr, low, high ); }
        }
        else if( type == "float" ) {
            float* ptr = &fp[ name ];
            float low = fr[ name ].first;
            float high = fr[ name ].second;
            if( uitype == "slider" ) { ui->addSliderf( name, ptr, low, high ); }
            else if( uitype == "dialer" ) { ui->addDialerf( name, ptr, low, high ); }
            else if( uitype == "ui" ) { data.emplace_back( MultiSlider::Data( name, ptr, low, high ) ); }
        }
        else if( type == "vec2" ) {
            vec2* ptr = &v2p[ name ];
            float low = v2r[ name ].first;
            float high = v2r[ name ].second;
            if( uitype == "range" ) { ui->addRangef( name, &ptr->x, &ptr->y, low, high ); }
            else if( uitype == "pad" ) { ui->addXYPad( name, ptr, XYPad::Format().min( vec2( low ) ).max( vec2( high ) ) ); }
            else if( uitype == "ui" ) {
                ui->addMultiSlider( name, {
                    MultiSlider::Data( name + "-X", &ptr->x, low, high ),
                    MultiSlider::Data( name + "-Y", &ptr->y, low, high )
                } );
            }
        }
        else if( type == "vec3" ) {
            vec3* ptr = &v3p[ name ];
            float low = v3r[ name ].first;
            float high = v3r[ name ].second;
            if( uitype == "ui" ) {
                ui->addMultiSlider( name, {
                    MultiSlider::Data( name + "-X", &ptr->x, low, high ),
                    MultiSlider::Data( name + "-Y", &ptr->y, low, high ),
                    MultiSlider::Data( name + "-Z", &ptr->z, low, high )
                } );
            }
        }
        else if( type == "vec4" ) {
            if( uitype == "ui" ) {
                vec4* ptr = &v4p[ name ];
                float low = v4r[ name ].first;
                float high = v4r[ name ].second;
                ui->addMultiSlider( name, {
                    MultiSlider::Data( name + "-X", &ptr->x, low, high ),
                    MultiSlider::Data( name + "-Y", &ptr->y, low, high ),
                    MultiSlider::Data( name + "-Z", &ptr->z, low, high ),
                    MultiSlider::Data( name + "-W", &ptr->w, low, high )
                } );
            }
            else if( uitype == "color" ) {
                ui->addColorPicker( name, &cp[ name ] );
            }
            
        }
    }
    
    if( data.size() ) {
        ui->addSpacer();
        ui->addMultiSlider( "UNIFORMS", data );
    }
}

//------------------------------------------------------------------------------
#pragma mark - COLOR PALETTE
//------------------------------------------------------------------------------

void ShaderToyApp::setupPalettes()
{
    mPaletteSurfRef = Surface32f::create( loadImage( getPalettesPath( "palettes.png" ) ) );
    mPaletteTexRef = gl::Texture2d::create( *mPaletteSurfRef.get(), gl::Texture2d::Format().minFilter( GL_LINEAR ).magFilter( GL_LINEAR ).loadTopDown().dataType( GL_FLOAT ).internalFormat( GL_RGBA ) );
}

//------------------------------------------------------------------------------
#pragma mark - SAVE IMAGE
//------------------------------------------------------------------------------

void ShaderToyApp::saveImage()
{
    mSaveImagePath = getRendersPath();
    mSaveImageName = "/ShaderToy " + toString( boost::posix_time::second_clock::universal_time() );
    mSaveImageExtension = "png";
    mSaveImageFrame = getElapsedFrames();
    mSaveImage = true;
}

void ShaderToyApp::saveImageAs()
{
    fs::path path = getSaveFilePath( getRendersPath() );
    if( path.string().length() ) {
        string fileName = path.filename().string();
        string ext = path.extension().string();
        string dir = path.parent_path().string() + "/";
        
        fs::path opath = fs::path( dir );
        auto it = fileName.find( "." );
        if( it != string::npos ) { fileName = fileName.substr( 0, it ); }
        vector<string> extensions = { "png", "jpg", "tif" };
        bool valid = false;
        for( auto it : extensions ) { if( it == ext ) { valid = true; break; } }
        if( !valid ) { ext = "png"; }
        
        mSaveImagePath = opath;
        mSaveImageName = fileName;
        mSaveImageExtension = ext;
        mSaveImageFrame = getElapsedFrames();
        mSaveImage = true;
    }
}

void ShaderToyApp::saveImage( const fs::path& path, const string& filename, const string &extension )
{
    //Save Settings
    fs::path pth = path; pth += filename; save( pth );

    ivec2 windowSize =  mOutputWindowRef->getSize();
    ivec2 outputSize = windowSize * mSaveImageSizeMultiplier;

    //Screen
    auto screen = Tiler::create( windowSize, windowSize, mOutputWindowRef, true );
    screen->setDrawBgFn( [ this ]( const vec2 &ul, const vec2 &ur, const vec2 &lr, const vec2 &ll ) {
        _drawOutput( ul, ur, lr, ll );
    } );

    //Render
    auto render = Tiler::create( outputSize, windowSize, mOutputWindowRef, true );
    render->setDrawBgFn( [ this ]( const vec2 &ul, const vec2 &ur, const vec2 &lr, const vec2 &ll ) {
        _drawOutput( ul, ur, lr, ll );
    } );
    
    if( createDirectory( path ) ) {
        fs::path low = path;
        low += fs::path( filename + "." + extension );
        writeImage( low, screen->getSurface() );
        
        fs::path high = path;
        high += fs::path( filename + " high." + extension );
        writeImage( high, render->getSurface() );
    }
}

//------------------------------------------------------------------------------
#pragma mark - MOVIE RECORDER
//------------------------------------------------------------------------------

void ShaderToyApp::setupRecorder()
{
    string fileName = "8 " + toString( boost::posix_time::second_clock::universal_time() );
    fs::path dir = getVideoPath( fileName );
    save( dir );
    fs::path path = dir;
    path += ".mov";
    mMovieExporterStartFrame = getElapsedFrames();
    
    if( mMovieExporterSaveMovie ) {
        auto format = qtime::MovieWriter::Format().codec( qtime::MovieWriter::JPEG ).fileType( qtime::MovieWriter::QUICK_TIME_MOVIE )
        .averageBitsPerSecond( 1000000000 ).defaultFrameDuration( 1.0 / 60.0 );
        mMovieExporter = qtime::MovieWriter::create( path, toPixels( mOutputWindowRef->getWidth() ), toPixels( mOutputWindowRef->getHeight() ), format );
    }
    
    if( mMovieExporterSaveFrames ) {
        ivec2 windowSize =  mOutputWindowRef->getSize();
        ivec2 outputSize = windowSize * mSaveImageSizeMultiplier;
        mSequenceTilerPath = dir;
        mSequenceTiler = Tiler::create( outputSize, windowSize, mOutputWindowRef, true );
        mSequenceTiler->setDrawBgFn( [ this ] ( glm::vec2 ul, glm::vec2 ur, glm::vec2 lr, glm::vec2 ll ) {
            _drawOutput( ul, ur, lr, ll );
        } );
    }
}

void ShaderToyApp::recordOutput()
{
    if( mMovieExporterRecording ) {
        int frameNumber = ( getElapsedFrames() - mMovieExporterStartFrame );
        if( frameNumber < mMovieExporterTotalFrames ) {
            mMovieExporterProgress = float( frameNumber ) / float( mMovieExporterTotalFrames );
            if( mMovieExporterSaveMovie ) {
                mMovieExporter->addFrame( copyWindowSurface() );
            }
            if( mMovieExporterSaveFrames ) {
                fs::path path = mSequenceTilerPath;
                path += "/Sequence ";
                path += toString( frameNumber );
                path += ".";
                path += "png";
                writeImage( path, mSequenceTiler->getSurface() );
            }
        } else {
            if( mMovieExporterSaveMovie ) {
                mMovieExporter->finish();
                mMovieExporter.reset();
            }
            mMovieExporterRecording = false;
            mMovieExporterProgress = 0.0f;
        }
    }
}

//------------------------------------------------------------------------------
#pragma mark - GLSL
//------------------------------------------------------------------------------

void ShaderToyApp::saveGlsl( const fs::path& path )
{
    fs::path glslFolder = path;
    glslFolder += "/Shaders";
    createDirectory( glslFolder );
    for( auto &it : mGlslPathMap ) {
        fs::path glslSubFolder = glslFolder;
        string glslFileName = it.first;
        size_t found = glslFileName.rfind( "/" );
        if( found != string::npos ) {
            fs::path temp = glslSubFolder;
            temp  += "/" + glslFileName.substr( 0, found );
            createDirectories( temp );
        }
        found = glslFileName.rfind( "." );
        if( found != string::npos ) {
            fs::path glslFile = glslSubFolder;
            glslFile += "/" + it.first;
            if( fs::exists( glslFile ) ) {
                fs::remove( glslFile );
            }
            fs::copy( getShadersPath( it.first ), glslFile );
        }
    }
}

void ShaderToyApp::loadGlsl( const fs::path& path )
{
    fs::path glslFolder = path;
    glslFolder += "/Shaders";
    for( auto &it : mGlslPathMap ) {
        fs::path newfile = glslFolder;
        newfile += "/" + it.first;
        fs::path oldFile = getShadersPath( it.first );
        if( fs::exists( oldFile ) ) {
            fs::remove( oldFile );
        }
        fs::copy( newfile, oldFile );
    }
    setupGlsl();
}

void ShaderToyApp::setupGlsl()
{
    mGlslPathMap[ "shader.vert" ] = getShadersPath( "shader.vert" );
    mGlslPathMap[ "shader.frag" ] = getShadersPath( "shader.frag" );
    auto vertShaderPath = mGlslPathMap[ "shader.vert" ];
    auto fragShaderPath = mGlslPathMap[ "shader.frag" ];
    try {
        mGlslLiveRef = LiveAssetManager::load( vertShaderPath, fragShaderPath, [ this, vertShaderPath, fragShaderPath ] ( DataSourceRef vertDataSource, DataSourceRef fragDataSource) {
            try {
                cout << "TIME: " << getElapsedSeconds() << endl;
                gl::ShaderPreprocessor pp;
                pp.addSearchDirectory( getShadersPath( "Common" ) );
                vector<string> sources = { pp.parse( vertShaderPath ), pp.parse( fragShaderPath ) };
                
                string name = "params";
                auto ui = mUIMap.find( name );
                if( ui != mUIMap.end() ) {
                    saveUI( getWorkingPath(), ui->second->getName() );
                    killUI( ui->second );
                    cout << "CALLING PARSE: " << endl;
                    mGlsParams.parseUniforms( sources );
                    addUI( setupShaderUI( createUI( name ) ) );
                    loadUI( getWorkingPath(), name );
                } else {
                    cout << "CALLING PARSE ELSE: " << endl;
                    mGlsParams.parseUniforms( sources );
                }
                
                mGlslRef = gl::GlslProg::create( sources[ 0 ], sources[ 1 ] );
                mGlsParams.applyUniforms( mGlslRef );
                if( mBatchRef ) {
                    mBatchRef->replaceGlslProg( mGlslRef );
                }
            } catch( Exception &exc ) {
                cout << "GLSL is wrong: " << exc.what() << endl;
            }
        });
    } catch( Exception &exc ) {
        cout << "GLSL Load Error: " << exc.what() << endl;
    }
}

CINDER_APP( ShaderToyApp, RendererGl( RendererGl::Options().msaa( 16 ) ), ShaderToyApp::prepareSettings )