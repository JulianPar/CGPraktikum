/*#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVector3D>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include <limits>

#include "Molecules.h"
void MyGLWidget::paintGL() {
    if (vboTrianglesId.size() == 0) return;

    if (viewMode == 1) {
        float aspect = (float) width()/height();
        projection.setToIdentity();
        projection.ortho(-aspect,aspect,-1.0,1.0,-10.0,10.0);
        modelView = RNow;
        modelView.scale(zoom,zoom,zoom);
        modelView.translate(-center);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0,0,width(),height());
        recursive_render(scene, scene->mRootNode,modelView);
    }

    if (viewMode == 2) {
         ovrSessionStatus sessionStatus;
         ovr_GetSessionStatus(session, &sessionStatus);
/*
         if (sessionStatus.IsVisible)
             std::cout << "visible" << std::endl;
         else
             std::cout << "not visible" << std::endl;
*/
         // Hand-Tracking
  /*       uint ConnectedControllerTypes = ovr_GetConnectedControllerTypes(session);
         ovrQuatf qNow = {0,0,0,1};
         // If controllers are connected, display their axis status.
         if (ConnectedControllerTypes & (ovrControllerType_LTouch | ovrControllerType_RTouch)) {
             double frameTiming = ovr_GetPredictedDisplayTime(session, frameIndex);
             ovrTrackingState trackState = ovr_GetTrackingState(session, frameTiming, ovrTrue);
             ovrPosef         handPoses[2];

            for (int hand = 0; hand < 2; ++hand) {
                 // Grab hand poses useful for rendering hand or controller representation
                 handPoses[hand]  = trackState.HandPoses[hand].ThePose;
                 std::cout << (hand==ovrHand_Left? "Left Hand Position:" : "Right Hand Position:")
                                                                  << " X = " << handPoses[hand].Position.x
                                                                  << " Y = " << handPoses[hand].Position.y
                                                                  << " Z = " << handPoses[hand].Position.z << std::endl;
                 std::cout << (hand==ovrHand_Left? "Left Hand Orientation:" : "Right Hand Orientation:")
                                                                  << " X = " << handPoses[hand].Orientation.x
                                                                  << " Y = " << handPoses[hand].Orientation.y
                                                                  << " Z = " << handPoses[hand].Orientation.z
                                                                  << " W = " << handPoses[hand].Orientation.w << std::endl;
                 qNow = handPoses[hand].Orientation;
             }
         }

         // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
         ovrEyeRenderDesc eyeRenderDesc[2];

         eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
         eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

         // Get eye poses, feeding in correct IPD offset
         ovrPosef    EyeRenderPose[2];
         ovrVector3f HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset, eyeRenderDesc[1].HmdToEyeOffset };
         for(int eye = 0; eye < 2; eye++) {
             ovrFovPort fov = hmdDesc.DefaultEyeFov[eye];

             std::cout << "defaultEyeFov = " << fov.DownTan << " " << fov.UpTan << " " << fov.RightTan << " " << fov.LeftTan << std::endl;
             std::cout << "HmdToEyeOffset[" << eye << "] = " << HmdToEyeOffset[eye].x << " " << HmdToEyeOffset[eye].y << " " << HmdToEyeOffset[eye].z << std::endl;
         }

         double sensorSampleTime; // sensorSampleTime is fed into the layer later
         ovr_GetEyePoses(session,frameIndex,ovrTrue,HmdToEyeOffset,EyeRenderPose,&sensorSampleTime);
         ovrVector3f le = EyeRenderPose[0].Position;
         ovrVector3f re = EyeRenderPose[1].Position;

        // Render Scene to Eye Buffers
        for(int eye = 0; eye < 2; eye++) {
            // Switch to eye render target
            // eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);
            GLuint curTexId;
            if (textureChain[eye]) {
                int curIndex;
                ovr_GetTextureSwapChainCurrentIndex(session,textureChain[eye], &curIndex);
                ovr_GetTextureSwapChainBufferGL(session,textureChain[eye], curIndex, &curTexId);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, fboId[eye]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthId[eye], 0);

            glViewport(0, 0, idealTextureSize.w, idealTextureSize.h);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_FRAMEBUFFER_SRGB);

            ovrMatrix4f P = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
            projection = QMatrix4x4(&P.M[0][0]);

            modelView.setToIdentity();
            ovrQuatf q = EyeRenderPose[eye].Orientation;
            modelView.rotate(QQuaternion(q.w,q.x,q.y,q.z).conjugate());
            ovrVector3f p = EyeRenderPose[eye].Position;
            modelView.translate(-p.x,-p.y,-p.z);
            modelView.rotate(QQuaternion(qNow.w, qNow.x, qNow.y, qNow.z));
            modelView.scale(0.25,0.25,0.25);
            modelView.scale(zoom,zoom,zoom);
            modelView.translate(-center);

            recursive_render(scene, scene->mRootNode,modelView);

            // Avoids an error when calling SetAndClearRenderSurface during next iteration.
            // Without this, during the next while loop iteration SetAndClearRenderSurface
            // would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
            // associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
            // eyeRenderTexture[eye]->UnsetRenderSurface();
            glBindFramebuffer(GL_FRAMEBUFFER, fboId[eye]);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, 0, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, 0, 0);

            // Commit changes to the textures so they get picked up frame
            // eyeRenderTexture[eye]->Commit();
            ovr_CommitTextureSwapChain(session,textureChain[eye]);
        }

        // Do distortion rendering, Present and flush/sync

        ovrLayerEyeFov ld;
        ld.Header.Type  = ovrLayerType_EyeFov;
        ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
        for(int eye = 0; eye < 2; ++eye) {
            ld.ColorTexture[eye] = textureChain[eye];
            ovrRecti viewport;
            viewport.Pos.x = viewport.Pos.y = 0;
            viewport.Size = idealTextureSize;
            ld.Viewport[eye]     = viewport;
            ld.Fov[eye]          = hmdDesc.DefaultEyeFov[eye];
            ld.RenderPose[eye]   = EyeRenderPose[eye];
            ld.SensorSampleTime  = sensorSampleTime;
        }

        ovrLayerHeader* layers = &ld.Header;
        ovrResult result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);

        // exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
        if (!OVR_SUCCESS(result)) {
            std::cout << "error when submitting frame" << std::endl;
            exit(0);
        }
        std::cout << "frameIndex = " << frameIndex << std::endl;
        frameIndex++;

        // Blit mirror texture to back buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
        // bind the default FBO of this QOpenGLWidget
        GLuint default_fbo = defaultFramebufferObject();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, default_fbo);

        GLint w = mirrorSize.w;
        GLint h = mirrorSize.h;
        qreal ratio = (qreal) w / h;
        GLint destHeight = width()/ratio;
        if (destHeight < height()) {
            GLint y0 = height()/2 - destHeight/2;
            GLint y1 = height()/2 + destHeight/2;
            glBlitFramebuffer(0, h, w, 0,
                              0, y0, width(), y1,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        } else {
            GLint destWidth = height()*ratio;
            GLint x0 = width()/2 - destWidth/2;
            GLint x1 = width()/2 + destWidth/2;
            glBlitFramebuffer(0, h, w, 0,
                              x0, 0, x1, height(),
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
}

void MyGLWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->delta();

    QVector3D u,v;
    pickLine(event->x(),event->y(),u,v);
    int model = -1,triangle = -1;

    double l = 0.0; // intersectTriangleSets(u,v,model,triangle);
    QVector3D vu = v - u ;
    if (triangle < 0) {
        QVector3D cu = center - u;
        l = QVector3D::dotProduct(vu,cu)/QVector3D::dotProduct(vu,vu);
    }
    QVector3D p = u + vu*l;

    double factor = (delta < 0)? 1.1 : 1/1.1;
    zoom *= factor;
    center = p + (center-p)/factor;

    update();
}
void MyGLWidget::initializeOVR() {
    ovrResult result = ovr_Initialize(NULL);
    std::cout << "ovr_Initialize = " << OVR_SUCCESS(result) << std::endl;

    ovrGraphicsLuid luid;
    result = ovr_Create(&session, &luid);
    std::cout << "ovr_Create = " << OVR_SUCCESS(result) << std::endl;

    hmdDesc = ovr_GetHmdDesc(session);
    std::cout << "hmd resolution = " << hmdDesc.Resolution.w << " " << hmdDesc.Resolution.h << std::endl;
    mirrorSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };

    // Make eye render buffers
    for (int eye = 0; eye < 2; ++eye) {
        idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
        std::cout << "idealTextureSize = " << idealTextureSize.w << " " << idealTextureSize.h << std::endl;

//      eyeRenderTexture[eye] = new TextureBuffer(session, true, true, idealTextureSize, 1, NULL, 1);

        ovrTextureSwapChainDesc desc = {};
        desc.Type = ovrTexture_2D;
        desc.ArraySize = 1;
        desc.Width = idealTextureSize.w;
        desc.Height = idealTextureSize.h;
        desc.MipLevels = 1;
        desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.SampleCount = 1;
        desc.StaticImage = ovrFalse;

        result = ovr_CreateTextureSwapChainGL(session,&desc,&textureChain[eye]);

        int length = 0;
        ovr_GetTextureSwapChainLength(session,textureChain[eye],&length);

        if (!OVR_SUCCESS(result)) {
            std::cout << "createTextureSwapChainGL failed" << std::endl;
            exit(0);
        }

        std::cout << "createTextureSwapChainGL o.k." << std::endl;
        std::cout << "chainLength = " << length << std::endl;

        for (int i = 0; i < length; ++i) {
            GLuint chainTexId;
            ovr_GetTextureSwapChainBufferGL(session, textureChain[eye], i, &chainTexId);
            glBindTexture(GL_TEXTURE_2D, chainTexId);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glGenFramebuffers(1, &fboId[eye]);

//      eyeDepthBuffer[eye] = new DepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);
        glGenTextures(1, &depthId[eye]);
        glBindTexture(GL_TEXTURE_2D, depthId[eye]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum internalFormat = GL_DEPTH_COMPONENT32F;
        GLenum type = GL_FLOAT;

//      GLenum internalFormat = GL_DEPTH_COMPONENT24;
//      GLenum type = GL_UNSIGNED_INT;
//      if (GLE_ARB_depth_buffer_float) {
//          internalFormat = GL_DEPTH_COMPONENT32F;
//          type = GL_FLOAT;
//      }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, idealTextureSize.w, idealTextureSize.h, 0, GL_DEPTH_COMPONENT, type, NULL);
    }

    ovrMirrorTextureDesc desc = {};
    // memset(&desc, 0, sizeof(desc));
    desc.Width = mirrorSize.w;
    desc.Height = mirrorSize.h;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

    // Create mirror texture and an FBO used to copy mirror texture to back buffer
    result = ovr_CreateMirrorTextureGL(session, &desc, &mirrorTexture);
    if (!OVR_SUCCESS(result)) {
        std::cout << "createMirrorTexture failed" << std::endl;
        exit(0);
    }

    // Configure the mirror read buffer
    ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &mirrorId);

    glGenFramebuffers(1, &mirrorFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorId, 0);
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_EyeLevel);
}*/
