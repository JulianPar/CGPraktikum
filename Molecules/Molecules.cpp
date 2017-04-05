#include <QApplication>
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



CGMainWindow::CGMainWindow (QWidget* parent) : QMainWindow (parent) {
    resize (720,720);

    QMenu *file = new QMenu("&File",this);
    file->addAction ("Load Hin data", this, SLOT(loadHin()), Qt::CTRL+Qt::Key_L);
    file->addAction ("Quit", qApp, SLOT(quit()), Qt::CTRL+Qt::Key_Q);

    menuBar()->addMenu(file);

    QMenu *view = new QMenu("&View",this);
    view->addAction("Orthogonal", this, SLOT(setOrthogonal()), Qt::CTRL+Qt::Key_O);
    view->addAction("Perspective", this, SLOT(setPerspective()), Qt::CTRL+Qt::Key_P);
    view->addAction("Stereo", this, SLOT(setStereo()), Qt::CTRL+Qt::Key_S);

    menuBar()->addMenu(view);

    QMenu *material = new QMenu("&Material",this);
    material->addAction("Brass", this, SLOT(setBrass()), Qt::CTRL+Qt::Key_O);
    material->addAction("Bronze", this, SLOT(setBronze()), Qt::CTRL+Qt::Key_P);
    material->addAction("Polished Bronze", this, SLOT(setPolishedBronze()), Qt::CTRL+Qt::Key_S);
    material->addAction("Chrome", this, SLOT(setChrome()), Qt::CTRL+Qt::Key_S);
    material->addAction("Copper", this, SLOT(setCopper()), Qt::CTRL+Qt::Key_S);
    material->addAction("Polished Copper", this, SLOT(setPolishedCopper()), Qt::CTRL+Qt::Key_S);
    material->addAction("Gold", this, SLOT(setGold()), Qt::CTRL+Qt::Key_S);
    material->addAction("Polished Gold", this, SLOT(setPolishedGold()), Qt::CTRL+Qt::Key_S);
    material->addAction("Pewter", this, SLOT(setPewter()), Qt::CTRL+Qt::Key_S);
    material->addAction("Silver", this, SLOT(setSilver()), Qt::CTRL+Qt::Key_S);
    material->addAction("Polished Silver", this, SLOT(setPolishedSilver()), Qt::CTRL+Qt::Key_S);
    material->addAction("Emerald", this, SLOT(setEmerald()), Qt::CTRL+Qt::Key_S);
    material->addAction("Jade", this, SLOT(setJade()), Qt::CTRL+Qt::Key_S);
    material->addAction("Obsidian", this, SLOT(setObsidian()), Qt::CTRL+Qt::Key_S);
    material->addAction("Pearl", this, SLOT(setPearl()), Qt::CTRL+Qt::Key_S);
    material->addAction("Ruby", this, SLOT(setRuby()), Qt::CTRL+Qt::Key_S);
    material->addAction("Turquoise", this, SLOT(setTurquoise()), Qt::CTRL+Qt::Key_S);
    material->addAction("Black Plastic", this, SLOT(setBlackPlastic()), Qt::CTRL+Qt::Key_S);
    material->addAction("Black Rubber", this, SLOT(setBlackRubber()), Qt::CTRL+Qt::Key_S);

    menuBar()->addMenu(material);

    QFrame* f = new QFrame (this);
    f->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    f->setLineWidth(2);

    ogl = new MyGLWidget (this,f);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(ogl);
    layout->setMargin(0);
    f->setLayout(layout);

    setCentralWidget(f);

    statusBar()->showMessage("Ready",1000);
}

CGMainWindow::~CGMainWindow () {}

void CGMainWindow::setOrthogonal() {
    ogl->viewMode = 1;
    ogl->update();
}

void CGMainWindow::setPerspective() {
    ogl->viewMode = 2;
    ogl->update();
}

void CGMainWindow::setStereo() {
    ogl->viewMode = 3;
    ogl->update();
}

void CGMainWindow::setBrass() {
    ogl->materialType = 0;
    ogl->update();
}

void CGMainWindow::setBronze() {
    ogl->materialType = 1;
    ogl->update();
}

void CGMainWindow::setPolishedBronze() {
    ogl->materialType = 2;
    ogl->update();
}

void CGMainWindow::setChrome() {
    ogl->materialType = 3;
    ogl->update();
}

void CGMainWindow::setCopper() {
    ogl->materialType = 4;
    ogl->update();
}

void CGMainWindow::setPolishedCopper() {
    ogl->materialType = 5;
    ogl->update();
}

void CGMainWindow::setGold() {
    ogl->materialType = 6;
    ogl->update();
}

void CGMainWindow::setPolishedGold() {
    ogl->materialType = 7;
    ogl->update();
}

void CGMainWindow::setPewter() {
    ogl->materialType = 8;
    ogl->update();
}

void CGMainWindow::setSilver() {
    ogl->materialType = 9;
    ogl->update();
}

void CGMainWindow::setPolishedSilver() {
    ogl->materialType = 10;
    ogl->update();
}

void CGMainWindow::setEmerald() {
    ogl->materialType = 11;
    ogl->update();
}

void CGMainWindow::setJade() {
    ogl->materialType = 12;
    ogl->update();
}

void CGMainWindow::setObsidian() {
    ogl->materialType = 13;
    ogl->update();
}

void CGMainWindow::setPearl() {
    ogl->materialType = 14;
    ogl->update();
}

void CGMainWindow::setRuby() {
    ogl->materialType = 15;
    ogl->update();
}

void CGMainWindow::setTurquoise() {
    ogl->materialType = 16;
    ogl->update();
}

void CGMainWindow::setBlackPlastic() {
    ogl->materialType = 17;
    ogl->update();
}

void CGMainWindow::setBlackRubber() {
    ogl->materialType = 18;
    ogl->update();
}

MyGLWidget::MyGLWidget(CGMainWindow *mainwindow,QWidget* parent ) : QOpenGLWidget (parent) {
    main = mainwindow;
}


void loadStlFile(std::vector<QVector3D>& vertices, const char *filename) {
    std::ifstream instream(filename,std::ios::binary);
    if (!instream) {
	std::cerr << "file does not exist!" << std::endl;
        return;
    }



    instream.seekg( 80, std::ios_base::beg ); // skip ascii header
    int trinum = 0;
    instream.read((char*) &trinum, 4 ); // number of vertices
    float tmp;
    for(int k = 0; k < trinum; k++) {
        for(int i=0;i < 3 ; i++ )
            instream.read( (char*) &tmp, 4 );
        for(int i = 0; i < 3; i++ ) {
            float v[3];
            for(int j = 0 ; j < 3 ; j++) {
                instream.read( (char*) &tmp, 4 );
                v[j] = tmp;
            }
            vertices.push_back(QVector3D(v[0],v[1],v[2]));
        }
        instream.read( (char*) &tmp, 2);
    }

    instream.close();
}

void CGMainWindow::loadModel() {
    QString fn = QFileDialog::getOpenFileName(this, "Load model ...", QString(), "STL files (*.stl)" );

    if (fn.isEmpty()) return;
    statusBar()->showMessage ("Loading model ...");

    std::vector<QVector3D> vertices;
    loadStlFile(vertices,fn.toLatin1());

    ogl->triangleSet.push_back(vertices);
    // ogl->initTrianglesVBO(vertices);
    ogl->initSmoothTrianglesVBO(vertices);
    ogl->updateBoundingBox(vertices);

    ogl->update();
    statusBar()->showMessage ("Loading model done.",3000);
}

void MyGLWidget::updateBoundingBox(const std::vector<QVector3D>& vertices) {
    for(int i=0;i<(int) vertices.size();i++) {
        for(int j=0;j<3;j++) {
            if (vertices[i][j] < bbMin[j]) bbMin[j] = vertices[i][j];
            if (vertices[i][j] > bbMax[j]) bbMax[j] = vertices[i][j];
        }
    }
    center = (bbMax+bbMin)/2;
    maxLen = bbMax[0]-bbMin[0];
    if (bbMax[1]-bbMin[1] > maxLen) maxLen = bbMax[1]-bbMin[1];
    if (bbMax[2]-bbMin[2] > maxLen) maxLen = bbMax[2]-bbMin[2];

//  std::cout << "center = " << center[0] << "," << center[1] << "," << center[2] << std::endl;
//  std::cout << "bbMin  = " << bbMin[0] << "," << bbMin[1] << "," << bbMin[2] << std::endl;
//  std::cout << "bbMax  = " << bbMax[0] << "," << bbMax[1] << "," << bbMax[2] << std::endl;

    zoom = 1.5/maxLen;
}

void MyGLWidget::refineSolidSphere(const std::vector<QVector3D>& sphere,std::vector<QVector3D>& refined) {
    for(size_t i=0;i<sphere.size()/3;i++) {
        const QVector3D& a = sphere[3*i+0];
        const QVector3D& b = sphere[3*i+1];
        const QVector3D& c = sphere[3*i+2];
        
        QVector3D ab = a+b;
        QVector3D bc = b+c;
        QVector3D ca = c+a;
        
        ab.normalize();
        bc.normalize();
        ca.normalize();
        
        refined.push_back(a);
        refined.push_back(ab);
        refined.push_back(ca);
        
        refined.push_back(ab);
        refined.push_back(b);
        refined.push_back(bc);
        
        refined.push_back(bc);
        refined.push_back(c);
        refined.push_back(ca);
        
        refined.push_back(ab);
        refined.push_back(bc);
        refined.push_back(ca);
    }
}

void MyGLWidget::initSolidSphereVBO() {
    std::vector<QVector3D> ico;
    float gr = 0.5*(1.0+sqrt(5.0));

    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,gr));
    ico.push_back( QVector3D(gr,-1.0,0.0)); 
    
    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(gr,-1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    
    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(0.0,gr,1.0));
    
    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(1.0,0.0,gr));
    
    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,gr,-1.0));
    
    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));
    ico.push_back( QVector3D(-gr,1.0,0.0));
    
    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(-gr,1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,-gr));
    
    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(0.0,-gr,-1.0));
    ico.push_back( QVector3D(0.0,-gr,1.0));
    
    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(0.0,-gr,1.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));
    
    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,-gr,-1.0));
    
    ico.push_back( QVector3D(1.0,0.0,gr));
    ico.push_back( QVector3D(-1.0,0.0,gr));
    ico.push_back( QVector3D(0.0,-gr,1.0));
    
    ico.push_back( QVector3D(1.0,0.0,gr));
    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));
    
    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(-gr,1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));
    
    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(-gr,1.0,0.0));
    
    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(-gr,1.0,0.0));
    
    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    
    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,-gr,-1.0));
    
    ico.push_back( QVector3D(0.0,-gr,-1.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    ico.push_back( QVector3D(gr,-1.0,0.0));
    
    ico.push_back( QVector3D(0.0,-gr,-1.0));
    ico.push_back( QVector3D(gr,-1.0,0.0));
    ico.push_back( QVector3D(0.0,-gr,1.0));
    
    ico.push_back( QVector3D(0.0,-gr,1.0));
    ico.push_back( QVector3D(gr,-1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,gr));

    for(size_t i=0;i<ico.size();i++) 
        ico[i].normalize();

    for(int i=0;i<2;i++) {
        std::vector<QVector3D> ico_refined;
        refineSolidSphere(ico,ico_refined);
        ico = ico_refined;
    }

    std::vector<QVector3D> vertexWithNormal;
    GLuint id;
    glGenBuffers(1,&id);

    for(size_t i=0;i<ico.size();i++) {
	vertexWithNormal.push_back(ico[i]);
	vertexWithNormal.push_back(ico[i]);
    }

    glBindBuffer(GL_ARRAY_BUFFER,id);
    glBufferData(GL_ARRAY_BUFFER,vertexWithNormal.size()*sizeof(QVector3D),vertexWithNormal.data(),GL_STATIC_DRAW);

    vboSolidSphereId = id;
    vboSolidSphereSize = static_cast<int>(ico.size());
}

//void MyGLWidget::initLineSphereVBO(){

//}

void MyGLWidget::initTrianglesVBO(const std::vector<QVector3D>& vertices) {
    std::vector<QVector3D> vertexWithNormal;
    GLuint id;
    glGenBuffers(1,&id);

    for(int i=0;i<(int) vertices.size();i+=3) {
        const QVector3D& a = vertices[i+0];
        const QVector3D& b = vertices[i+1];
        const QVector3D& c = vertices[i+2];
        QVector3D n = QVector3D::crossProduct(b-a,c-a);
        n.normalize();
        vertexWithNormal.push_back(a);
        vertexWithNormal.push_back(n);

        vertexWithNormal.push_back(b);
        vertexWithNormal.push_back(n);

        vertexWithNormal.push_back(c);
        vertexWithNormal.push_back(n);
    }

    glBindBuffer(GL_ARRAY_BUFFER,id);
    glBufferData(GL_ARRAY_BUFFER,vertexWithNormal.size()*sizeof(QVector3D),vertexWithNormal.data(),GL_DYNAMIC_DRAW);

    vboTriangleSetId.push_back(id);
}

void MyGLWidget::initSmoothTrianglesVBO(const std::vector<QVector3D>& vertices) {
    int nv = (int) vertices.size();

    std::vector<QVector3D> normals(nv);

    for(int i=0;i<nv;i+=3) {
        const QVector3D& a = vertices[i+0];
        const QVector3D& b = vertices[i+1];
        const QVector3D& c = vertices[i+2];
        QVector3D n = QVector3D::crossProduct(b-a,c-a);
        n.normalize();

        normals[i+0] = n;
        normals[i+1] = n;
        normals[i+2] = n;
    }

    std::vector<int> vid(nv);

    for(int i=0;i<nv;i++)
        vid[i] = i;

    struct VertexComparator {
        const std::vector<QVector3D>& vertices;
    
        VertexComparator(const std::vector<QVector3D>& v) : vertices(v) {}
    
        bool operator()(int i,int j) {
            if (vertices[i].x() < vertices[j].x()) return true;
            if (vertices[i].x() > vertices[j].x()) return false;
            if (vertices[i].y() < vertices[j].y()) return true;
            if (vertices[i].y() > vertices[j].y()) return false;
            if (vertices[i].z() < vertices[j].z()) return true;
            return false;
        }
    } vertexLess(vertices);

    std::sort(vid.begin(),vid.end(),vertexLess);

    float eps = 1e-6;
    int l = 0, r = 0;
    while (r < nv) {
        do {
            r++;
        } while ((r < nv) && (vertices[vid[l]].distanceToPoint(vertices[vid[r]]) < eps));

        QVector3D s(0,0,0);
        for(int i=l;i<r;i++)
            s += normals[vid[i]];

        s.normalize();

        for(int i=l;i<r;i++) 
            normals[vid[i]] = s;

        l = r;
    }

    std::vector<QVector3D> vertexWithNormal;
    GLuint id;
    glGenBuffers(1,&id);

    for(int i=0;i<(int) vertices.size();i++) {
        vertexWithNormal.push_back(vertices[i]);
        vertexWithNormal.push_back(normals[i]);
    }

    glBindBuffer(GL_ARRAY_BUFFER,id);
    glBufferData(GL_ARRAY_BUFFER,vertexWithNormal.size()*sizeof(QVector3D),vertexWithNormal.data(),GL_DYNAMIC_DRAW);

    vboTriangleSetId.push_back(id);
}

void MyGLWidget::initShaders() {
    setlocale(LC_NUMERIC,"C");

    if (!p_Phong.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/VertexShaderPhong.glsl"))
        close();

    if (!p_Phong.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FragmentShaderPhong.glsl"))
        close();

    if (!p_Phong.link())
        close();

    if (!p_Phong.bind())
        close();

    setlocale(LC_ALL,"");
}

void MyGLWidget::initMaterials() {
    float mat[19][11] = { 
        { 0.329412,0.223529,0.027451,0.780392,0.568627,0.113725,0.992157,0.941176,0.807843,27.8974,1.0},
        { 0.2125,0.1275,0.054,0.714,0.4284,0.18144,0.393548,0.271906,0.166721,25.6,1.0 },
        { 0.25,0.148,0.06475,0.4,0.2368,0.1036,0.774597,0.458561,0.200621,76.8,1.0 },
        { 0.25,0.25,0.25,0.4,0.4,0.4,0.774597,0.774597,0.774597,76.8,1.0 },
        { 0.19125,0.0735,0.0225,0.7038,0.27048,0.0828,0.256777,0.137622,0.086014,12.8,1.0 },
        { 0.2295,0.08825,0.0275,0.5508,0.2118,0.066,0.580594,0.223257,0.0695701,51.2,1.0 },
        { 0.24725,0.1995,0.0745,0.75164,0.60648,0.22648,0.628281,0.555802,0.366065,51.2,1.0 },
        { 0.24725,0.2245,0.0645,0.34615,0.3143,0.0903,0.797357,0.723991,0.208006,83.2,1.0 },
        { 0.105882,0.058824,0.113725,0.427451,0.470588,0.541176,0.333333,0.333333,0.521569,9.84615,1.0 },
        { 0.19225,0.19225,0.19225,0.50754,0.50754,0.50754,0.508273,0.508273,0.508273,51.2,1.0 },
        { 0.23125,0.23125,0.23125,0.2775,0.2775,0.2775,0.773911,0.773911,0.773911,89.6,1.0 },
        { 0.0215,0.1745,0.0215,0.07568,0.61424,0.07568,0.633,0.727811,0.633,76.8,0.55 },
        { 0.135,0.2225,0.1575,0.54,0.89,0.63,0.316228,0.316228,0.316228,12.8,0.95 },
        { 0.05375,0.05,0.06625,0.18275,0.17,0.22525,0.332741,0.328634,0.346435,38.4,0.82 },
        { 0.25,0.20725,0.20725,1.0,0.829,0.829,0.296648,0.296648,0.296648,11.264,0.922 },
        { 0.1745,0.01175,0.01175,0.61424,0.04136,0.04136,0.727811,0.626959,0.626959,76.8,0.55 },
        { 0.1,0.18725,0.1745,0.396,0.74151,0.69102,0.297254,0.30829,0.306678,12.8,0.8 },
        { 0.0,0.0,0.0,0.01,0.01,0.01,0.50,0.50,0.50,32,1.0 },
        { 0.02,0.02,0.02,0.01,0.01,0.01,0.4,0.4,0.4,10,1.0 }
    };

    for(int i=0;i<19;i++)
        for(int j=0;j<11;j++)
            material[i][j] = mat[i][j];
}

void MyGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    initShaders();
    initMaterials();
    initSolidSphereVBO();

    glClearColor(0.5,0.5,0.5,1);
    // glClearColor(0,0,0,1);

    glEnable(GL_DEPTH_TEST);

    bbMin[0] = bbMin[1] = bbMin[2] =  std::numeric_limits<double>::infinity();
    bbMax[0] = bbMax[1] = bbMax[2] = -std::numeric_limits<double>::infinity();
    center[0] = center[1] = center[2] = 0.0;
    zoom = 1.0;
}

void MyGLWidget::drawSolidSphere(const QVector3D& c, float r) {
    QMatrix4x4 M(modelView);
    M.translate(c); 
    M.scale(r);

    const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uMVMat", M);
    p_Phong.setUniformValue("uNMat", M.normalMatrix());
    p_Phong.setUniformValue("uAmbient",QVector3D(material[t][0],material[t][1],material[t][2]));
    p_Phong.setUniformValue("uDiffuse",QVector3D(material[t][3],material[t][4],material[t][5]));
    p_Phong.setUniformValue("uSpecular",QVector3D(material[t][6],material[t][7],material[t][8]));
    p_Phong.setUniformValue("uShininess",material[t][9]);

    glBindBuffer(GL_ARRAY_BUFFER, vboSolidSphereId);
    int vertexLocation = p_Phong.attributeLocation("a_position");
    p_Phong.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    int normalLocation = p_Phong.attributeLocation("a_normal");
    p_Phong.enableAttributeArray(normalLocation);
	glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_TRIANGLES,0,vboSolidSphereSize);
}

void MyGLWidget::drawTriangleSets() {
    const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uPMat", projection);
    p_Phong.setUniformValue("uMVMat", modelView);
    p_Phong.setUniformValue("uNMat", modelView.normalMatrix());
    p_Phong.setUniformValue("uAmbient",QVector3D(material[t][0],material[t][1],material[t][2]));
    p_Phong.setUniformValue("uDiffuse",QVector3D(material[t][3],material[t][4],material[t][5]));
    p_Phong.setUniformValue("uSpecular",QVector3D(material[t][6],material[t][7],material[t][8]));
    p_Phong.setUniformValue("uShininess",material[t][9]);

    for(int i=0;i<(int) triangleSet.size();i++) {
        glBindBuffer(GL_ARRAY_BUFFER,vboTriangleSetId[i]);
    
        int vertexLocation = p_Phong.attributeLocation("a_position");
        p_Phong.enableAttributeArray(vertexLocation);
        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    
        int normalLocation = p_Phong.attributeLocation("a_normal");
        p_Phong.enableAttributeArray(normalLocation);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const GLvoid*) (3*sizeof(float)));
    
        glDrawArrays(GL_TRIANGLES,0,triangleSet[i].size());
    }
}

double MyGLWidget::intersectTriangle(const QVector3D& p0,const QVector3D& p1,const QVector3D& a,const QVector3D& b,const QVector3D& c) {
    QMatrix4x4 M(p0[0]-p1[0],b[0]-a[0],c[0]-a[0],0.0,
                 p0[1]-p1[1],b[1]-a[1],c[1]-a[1],0.0,
                 p0[2]-p1[2],b[2]-a[2],c[2]-a[2],0.0,
                 0.0,        0.0,      0.0,      1.0);
    double inf = std::numeric_limits<double>::infinity();

    bool invertible;
    QMatrix4x4 M1 = M.inverted(&invertible);
    if (!invertible) return inf;

    QVector4D l = M1 * QVector4D(p0[0]-a[0],p0[1]-a[1],p0[2]-a[2],1.0);

    if (l[0] < 0.0) return inf;
    if (l[1] < 0.0) return inf;
    if (l[2] < 0.0) return inf;
    if (l[1] > 1.0) return inf;
    if (l[2] > 1.0) return inf;
    if (l[1]+l[2] > 1.0) return inf;
 
    return l[0];
}

double MyGLWidget::intersectTriangleSets(const QVector3D& p0,const QVector3D& p1,int& model,int& triangle) {
    double lmin = std::numeric_limits<double>::infinity();
    int imin = -1, jmin = -1;

    for(int i=0;i<(int) triangleSet.size();i++) {
        for(int j=0;j<(int) triangleSet[i].size();j+=3) {
            const QVector3D& a = triangleSet[i][j+0];
            const QVector3D& b = triangleSet[i][j+1];
            const QVector3D& c = triangleSet[i][j+2];
            double l = intersectTriangle(p0,p1,a,b,c);

            if (l < lmin) {
                lmin = l;
                imin = i;
                jmin = j;
            }
        }
    }

    // std::cout << "imin = " << imin << "  jmin = " << jmin << "  lmin = " << lmin << std::endl;

    return lmin;
}

QVector2D MyGLWidget::worldCoord(int x, int y) {
    double dx,dy;

    if (width() > height()) {
        dx = (2.0*x-width())/height();
        dy = 1.0-2.0*y/(double) height();
    } else {
        dx = 2.0*x/(double) width()-1.0;
        dy = (height()-2.0*y)/width();
    }

    return QVector2D(dx,dy);
}

QVector3D MyGLWidget::mouseToTrackball(int x, int y) {
    QVector2D u = worldCoord(x,y);
    QVector3D v(u[0],u[1],0.0);
    double d = v[0]*v[0]+v[1]*v[1];
    if (d > 1.0)
         v /= sqrt(d);
    else v[2] = sqrt(1.0-d*d);

    return v;
}

/*
QQuaternion MyGLWidget::trackball(const QVector3D& u, const QVector3D& v) {
    QVector3D uxv = QVector3D::crossProduct(u,v);
    double uv = QVector3D::dotProduct(u,v);
    QQuaternion ret(1+uv,uxv);
    ret.normalize();
    return ret;
}
*/

QMatrix4x4 MyGLWidget::trackball(const QVector3D& u, const QVector3D& v) {
    QVector3D uxv = QVector3D::crossProduct(u,v);
    double sphi = uxv.length();
    double cphi = QVector3D::dotProduct(u,v);
    double phi = atan2(sphi,cphi);
    uxv.normalize();
    QMatrix4x4 R;
    R.setToIdentity();
    R.rotate(180.0*phi/M_PI,uxv);
    return R;
}

void MyGLWidget::paintGL() {
    modelView = RNow;
//  modelView.rotate(qNow);
//  modelView.rotate(theta,1.0,0.0,0.0);
//  modelView.rotate(phi,0.0,1.0,0.0);
    modelView.scale(15,15,15);
    modelView.scale(zoom,zoom,zoom);
    modelView.translate(-center);

    float mynear = 10;
    float myfar = 100;
    float aspectratio = width()/(float) height();
    float displayHeight = 30;
    float displayWidth = aspectratio*displayHeight;
    float camX = 0;
    float camY = 0;
    float camZ = 50;
    float left   = mynear*(-displayWidth/2-camX)/camZ; 
    float right  = mynear*( displayWidth/2-camX)/camZ;
    float bottom = mynear*(-displayHeight/2-camY)/camZ; 
    float top    = mynear*( displayHeight/2-camY)/camZ;

    projection.setToIdentity();
    if (viewMode == 1)
        projection.ortho(-displayWidth/2,displayWidth/2,-displayHeight/2,displayHeight/2,mynear,myfar);
    if (viewMode == 2)
        projection.frustum(left,right,bottom,top,mynear,myfar);
    projection.translate(QVector3D(-camX,-camY,-camZ));

    camX = -3;
    left   = mynear*(-displayWidth/2-camX)/camZ; 
    right  = mynear*( displayWidth/2-camX)/camZ;
    bottom = mynear*(-displayHeight/2-camY)/camZ; 
    top    = mynear*( displayHeight/2-camY)/camZ;

    QMatrix4x4 PLeft;
    PLeft.frustum(left,right,bottom,top,mynear,myfar);
    PLeft.translate(QVector3D(-camX,-camY,-camZ));

    camX = 3;
    left   = mynear*(-displayWidth/2-camX)/camZ; 
    right  = mynear*( displayWidth/2-camX)/camZ;
    bottom = mynear*(-displayHeight/2-camY)/camZ; 
    top    = mynear*( displayHeight/2-camY)/camZ;

    QMatrix4x4 PRight;
    PRight.frustum(left,right,bottom,top,mynear,myfar);
    PRight.translate(QVector3D(-camX,-camY,-camZ));

    glColorMask(true,true,true,true);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //drawSolidSphere(QVector3D(0,0,0),1);

    if (viewMode == 3) {
        glColorMask(true,false,false,true);
        projection = PLeft;
        drawTriangleSets();

        glClear(GL_DEPTH_BUFFER_BIT);

        glColorMask(false,false,true,true);
        projection = PRight;
        drawTriangleSets();
    } else {

    if(m.parts.size()!=0)


        for(int i=0;i<m.parts.size();i++){
            for(int j=0;j<m.parts[i].elements.size();j++){
            drawSolidSphere(m.parts.at(i).elements[j].pos,0.5);
            }
        }

    if (m.parts.size()==0)
        drawSolidSphere(QVector3D(0,0,0),1);
    drawTriangleSets();
    }


}

void MyGLWidget::resizeGL(int width, int height) {
    glViewport(0,0,width,height);
    update();
}

void MyGLWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->delta();

    QVector3D u,v;
    pickLine(event->x(),event->y(),u,v);
    int model = -1,triangle = -1;
    double l = intersectTriangleSets(u,v,model,triangle);
    QVector3D vu = v - u ;
    if (triangle < 0) {
        QVector3D cu = center - u;
        l = QVector3D::dotProduct(vu,cu)/QVector3D::dotProduct(vu,vu);
    }
    QVector3D p = u + vu*l;

    double factor = (delta > 0)? 1.1 : 1/1.1;
    zoom *= factor;
    center = p + (center-p)/factor;

    update();
}

void MyGLWidget::pickLine(int x, int y, QVector3D& p0, QVector3D& p1) {
    QVector2D p((2.0*x)/width()-1.0,1.0-(2.0*y)/height());

    QMatrix4x4 PM = projection*modelView;
    QVector4D a = PM.inverted()*QVector4D(p[0],p[1],-1.0,1.0);
    QVector4D b = PM.inverted()*QVector4D(p[0],p[1],1.0,1.0);
    
    p0 = a.toVector3DAffine();
    p1 = b.toVector3DAffine();
}

void MyGLWidget::mousePressEvent(QMouseEvent *event) {
    button = event->button();
    mouseX = event->x();
    mouseY = event->y();

    QVector3D p0,p1;
    pickLine(mouseX,mouseY,p0,p1);


    update();
}

void MyGLWidget::mouseReleaseEvent(QMouseEvent*) {}

void MyGLWidget::mouseMoveEvent(QMouseEvent* event) {
    if (button == Qt::LeftButton) {
        QVector3D p1 = mouseToTrackball(mouseX,mouseY);
        QVector3D p2 = mouseToTrackball(event->x(),event->y());

//      phi += (event->x()-mouseX)/5.0;
//      theta += (event->y()-mouseY)/5.0;
//      if (theta < -90.0) theta = -90.0;
//      if (theta >  90.0) theta =  90.0;

//      QQuaternion q = trackball(p1,p2);
//      qNow = q * qNow;
//      qNow.normalize();

        RNow = trackball(p1,p2) * RNow;
    }

    if (button == Qt::RightButton) {
        QVector3D u0,u1,v0,v1;
        QVector3D vu,cu,p0,p1;
        pickLine(mouseX,mouseY,u0,v0);
        int model = -1,triangle = -1;
        vu = v0 - u0;
        double l = intersectTriangleSets(u0,v0,model,triangle);
        if (triangle < 0) {
            cu = center - u0;
            l = QVector3D::dotProduct(vu,cu)/QVector3D::dotProduct(vu,vu);
        }
        p0 = u0 + vu*l;

        pickLine(event->x(),event->y(),u1,v1);

        vu = v1 - u1;
        if (triangle < 0) 
            cu = center - u1;
        else
            cu = p0 - u1;
        l = QVector3D::dotProduct(vu,cu)/QVector3D::dotProduct(vu,vu);
        p1 = u1 + vu*l;

        center -= p1 - p0;
    }

    mouseX = event->x();
    mouseY = event->y();

    update();
}

int main (int argc, char **argv) {
    QApplication app(argc, argv);

    CGMainWindow w(NULL);

    w.show();

    return app.exec();
}
