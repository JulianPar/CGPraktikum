#ifndef VRINSPECTOR_H
#define VRINSPECTOR_H

#include <QMainWindow>
#include <QGridLayout>
#include <QFrame>
#include <QtCore>
#include <QLocale>
#include <QInputDialog>

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

#include <assimp/scene.h>

#include <OVR_CAPI_GL.h>

struct atom{
    int index;
    std::string name;
    QVector3D pos;
    float radius;
    std::vector<std::pair<int,std::string>> bonds;
    QVector3D color;

};

struct residue{
    std::string rname;
    std::vector<atom> elements;
};

struct molecule{
    std::string name;
    std::vector<residue> parts;
    std::vector<QVector3D> positions;
};

struct structure{
    std::vector<molecule> mols;
};

class MyGLWidget;

class CGMainWindow : public QMainWindow {
    Q_OBJECT

public slots:

    void loadHin();
    void loadModel();
    void setOrthogonal();
    void setOculus();

public:

    CGMainWindow (QWidget* parent = 0);
    ~CGMainWindow ();
    MyGLWidget *ogl;
};

class MyGLWidget : public QOpenGLWidget, public QOpenGLFunctions_3_0 {
    Q_OBJECT

public slots:


    void centerEye();

public:
    MyGLWidget(CGMainWindow*,QWidget*);
    void initShaders();
    void initializeGL();
    void initializeOVR();
    void initFBO();
    void initCubeMap(QString);
    void initCubeVBOandIBO();
    void pressA();
    void pressB();
    void refineSolidSphere(const std::vector<QVector3D>&,std::vector<QVector3D>&);
    void initSolidSphereVBO();
    void initCylinderVBO();

    void pickLine(int,int,QVector3D&,QVector3D&);
    double intersectTriangle(const QVector3D&,const QVector3D&,const QVector3D&,const QVector3D&,const QVector3D&);
    // double intersectTriangleSets(const QVector3D&,const QVector3D&,int&,int&);

    void loadModel(QString filename);
    void initMeshVBOandIBO(const aiScene*);
    void recursive_render(int,const aiNode*,QMatrix4x4);

    QVector2D worldCoord(int,int);
    QVector3D mouseToTrackball(int,int);
    QMatrix4x4 trackball(const QVector3D&, const QVector3D&);

    float zoom;
    QVector3D center,bbMin,bbMax, eyeCenter;
    QMatrix4x4 RNow;
    float phi,theta;
    std::vector<float> phiMol, thetaMol;
    int selectIndex=0;
    bool buttonAPressed,buttonBPressed;

    std::vector<atom> atoms;
    structure struc;
    bool flag;

    std::vector<const aiScene*> scenes;
    std::vector<std::vector<unsigned int> > vboTrianglesId, vboTrianglesSize, iboTrianglesId, iboTrianglesSize;

    int viewMode = 1, oldViewMode = 1;

    std::vector<std::vector<QOpenGLTexture*> > textures;

    std::vector<GLuint> vboTriangleSetId;
    GLuint vboSolidSphereId, vboCylinderId;
    int vboSolidSphereSize, vboCylinderSize;
    GLuint vboCubeId, iboCubeId, vboCubeSize, iboCubeSize;

protected:

    void paintGL();

    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);

    void drawSolidSphere(const QVector3D&,float,const QVector3D);
    void drawCylinder(const QVector3D&,const QVector3D&,float,const QVector3D);
    void drawMolecule(molecule mol);
    void drawCubeMap();

    CGMainWindow *main;
    QTimer *timer;
    int mouseX,mouseY,button;

private:

    ovrSession session;
    ovrHmdDesc hmdDesc;
    ovrTextureSwapChain textureChain[2];
    ovrMirrorTexture mirrorTexture;
    GLuint depthId[2];
    GLuint fboId[2];
    GLuint mirrorId, mirrorFBO;
    ovrSizei idealTextureSize;
    ovrSizei mirrorSize; // The size of the texture containing a mirror view of the HMD display
    long long frameIndex = 0;

    QOpenGLShaderProgram p_Diffuse;
    QOpenGLShaderProgram p_Phong;
    QMatrix4x4 projection,modelView;
    QOpenGLShaderProgram p_Cube;
    QOpenGLTexture *cubemap;
};

#endif
