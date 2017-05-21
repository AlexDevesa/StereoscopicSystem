#include "Common.h"

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Serial.h"
#include <tchar.h>
// *********************************  1) SELECCIONAR ORDEN DE CÁMARA ESQUERDA/DEREITA
// *********************************  2) SELECCIONAR PORTO COM ARDUINO



// *************		(1)


int CAMARA_OJO[2] = { 0, 0 };
//int CAMARA_OJO[2] = { 2,1 };
//int CAMARA_OJO[2] = { 1, 2 };

struct datoCapturado {
  ovrPosef pose;
  cv::Mat imagen;
};
bool parado{ false };
float yaw, pitch, roll;
bool reset = true;
float                   eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
float                   ipd{ OVR_DEFAULT_IPD };
int contador = 0;


class manipuladorCam {

private:
  bool Frame{ false };
  
  cv::VideoCapture videoCapture;
  std::thread hiloCaptura;
  std::thread hiloTransmite;
  std::mutex mutex;
  std::mutex mutex2;
  datoCapturado frame;
  ovrHmd hmd;
  short contador=0;
  CSerial arduino;


public:
	
  manipuladorCam() {
  }

 

  float comienzaCaptura(ovrHmd & hmdRef, int cam) {
	
	hmd = hmdRef;
    videoCapture.open(cam);
    if (!videoCapture.isOpened()) {
      FAIL("No se puede recibir de la cámara %i", cam);
    }
	//videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
	//videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1080);
    for (int i = 0; i < 10 && !videoCapture.read(frame.imagen); i++) {
      Platform::sleepMillis(10);
    }
    if (!videoCapture.read(frame.imagen)) {
      FAIL("No se puede abrir el primer frmae de la cámara %i", cam);
    }
    float formato = (float)frame.imagen.cols / (float)frame.imagen.rows;


	// *************		(2)

	arduino.Open(_T("COM4"), 0, 0, false);
	arduino.Setup(CSerial::EBaud9600, CSerial::EData8, CSerial::EParNone, CSerial::EStop1);
	arduino.SetupHandshaking(CSerial::EHandshakeHardware);

    hiloCaptura = std::thread(&manipuladorCam::bucleCaptura, this);
	hiloTransmite = std::thread(&manipuladorCam::transmite, this);
    Platform::sleepMillis(200);
    return formato;
  }

  void pararCaptura() {
    parado = true;
    hiloCaptura.join();
    videoCapture.release();
  }

  void set(const datoCapturado & newFrame) {
    std::lock_guard<std::mutex> guard(mutex);
    frame = newFrame;
    Frame = true;
  }

  bool get(datoCapturado & out) {
    if (!Frame) {
      return false;
    }
    std::lock_guard<std::mutex> guard(mutex);
    out = frame;
    Frame = false;
    return true;
  }

  void bucleCaptura() {
    datoCapturado captured;
	
	while (!parado) {
		
		float tiempoCaptura = ovr_GetTimeInSeconds();
		ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, tiempoCaptura);
		captured.pose = ts.HeadPose.ThePose;

		videoCapture.read(captured.imagen);
		cv::flip(captured.imagen.clone(), captured.imagen, 0);
		set(captured);

		if (reset == true){
			ovrHmd_RecenterPose(hmd);
			reset = false;
		}
		if (contador == 2){
			OVR::Posef pose = ts.HeadPose.ThePose;
			pose.Rotation.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>
				(&yaw, &pitch, &roll);
			yaw = OVR::RadToDegree(yaw);
			pitch = OVR::RadToDegree(pitch);			
		}
		else{contador++;}
    }
  }
  void transmite(){ // TRAMA: ':yaw$pitch/'
	  int eyaw, epitch;
	  char buffer1[20];
	  while (!parado){
		  eyaw = (int)yaw;
		  epitch = (int)pitch;
		  eyaw = eyaw + 90;
		  epitch = epitch + 90;
		  if (eyaw < 0){ eyaw = 0; }
		  if (eyaw > 180){ eyaw = 180; }
		  if (epitch < 0){ epitch = 0; }
		  if (epitch > 180){ epitch = 180; }
		  sprintf(buffer1, ":%i$%i/", eyaw, epitch);
		  arduino.Write(buffer1);
		  SAY("%i...........%i", eyaw, epitch);
		  Sleep(50); 
	  }
  }
}; 

class CamAPP : public RiftApp {
protected:
  ProgramPtr programa;
  TexturePtr textura[2];
  ShapeWrapperPtr geometriaVideo[2];
  manipuladorCam manipuladorCaptura[2];
  datoCapturado captureData[2];
  
  //int state = glfwGetKey(window, GLFW_KEY_C);
public:
  CamAPP() {
  }

  virtual ~CamAPP() {
    for (int i = 0; i < 2; i++) {
      manipuladorCaptura[i].pararCaptura();
    }
  }

  void initGl() {
    RiftApp::initGl();
	
    using namespace oglplus;

    programa = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, 
		Resource::SHADERS_TEXTURED_FS);
    for (int i = 0; i < 2; i++) {
      textura[i] = TexturePtr(new Texture());
      Context::Bound(TextureTarget::_2D, *textura[i])
        .MagFilter(TextureMagFilter::Linear)
        .MinFilter(TextureMinFilter::Linear);
      programa = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, 
		  Resource::SHADERS_TEXTURED_FS);
      float aspect = manipuladorCaptura[i].comienzaCaptura(hmd, CAMARA_OJO[i]);
      geometriaVideo[i] = oria::loadPlane(programa, aspect);
	}
  }

  virtual void update() {
    for (int i = 0; i < 2; i++) {
      if (manipuladorCaptura[i].get(captureData[i])) {
        using namespace oglplus;
        Context::Bound(TextureTarget::_2D, *textura[i])
          .Image2D(0, PixelDataInternalFormat::RGBA8,
          captureData[i].imagen.cols, captureData[i].imagen.rows, 0,
          PixelDataFormat::BGR, PixelDataType::UnsignedByte,
          captureData[i].imagen.data);

      }
    }
  }
  
  virtual void renderScene() {
    using namespace oglplus;
    glClear(GL_DEPTH_BUFFER_BIT);
    //oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    MatrixStack & ms = Stacks::modelview();
	 
    ms.withPush([&] { //usar transformacións comentadas, crean lag 
      //glm::quat Pose = ovr::toGlm(getEyePose().Orientation);
     // glm::quat camPose = ovr::toGlm(captureData[getCurrentEye()].pose.Orientation);
      //glm::mat4 camD = glm::mat4_cast(glm::inverse(Pose) * camPose);
		
		
	 ms.identity();
     // ms.preMultiply(camD);
	
	  ms.translate(glm::vec3(0, 0, -2.75));

      textura[getCurrentEye()]->Bind(TextureTarget::_2D); 
      oria::renderGeometry(geometriaVideo[getCurrentEye()], programa);
	 

	  //if (state = GLFW_PRESS){
		//ovrHmd_RecenterPose(hmd);
		//SAY("......................................................................");
	  //}


    });
    oglplus::DefaultTexture().Bind(TextureTarget::_2D);
  }
};

RUN_OVR_APP(CamAPP);
