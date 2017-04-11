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
#include <QObject>
#include <OVR_CAPI_GL.h>


#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "OculusInspector.h"




CGMainWindow::CGMainWindow (QWidget* parent) : QMainWindow (parent) {

    resize (720,720);

    QMenu *file = new QMenu("&File",this);
    file->addAction ("Load Hin file", this, SLOT(loadHin()), Qt::CTRL+Qt::Key_L);
    file->addAction ("Quit", qApp, SLOT(quit()), Qt::CTRL+Qt::Key_Q);

    menuBar()->addMenu(file);

    QMenu *view = new QMenu("&View",this);
    view->addAction("Orthogonal", this, SLOT(setOrthogonal()), Qt::CTRL+Qt::Key_O);
    view->addAction("Oculus", this, SLOT(setOculus()), Qt::CTRL+Qt::Key_C);

    QFrame* f = new QFrame (this);
    f->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    f->setLineWidth(2);

    ogl = new MyGLWidget (this,f);

    view->addAction("Center", ogl, SLOT(centerEye()), Qt::CTRL+Qt::Key_B);

    menuBar()->addMenu(view);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(ogl);
    layout->setMargin(0);
    f->setLayout(layout);

    setCentralWidget(f);

    statusBar()->showMessage("Ready",1000);
}

CGMainWindow::~CGMainWindow () {}

void CGMainWindow::setOrthogonal() {
    ogl->oldViewMode = ogl->viewMode;
    ogl->viewMode = 1;
    ogl->update();
}

void CGMainWindow::setOculus() {
    ogl->oldViewMode = ogl->viewMode;
    ogl->viewMode = 2;
    ogl->update();
}

void MyGLWidget::centerEye() {
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
    ovrVector3f l = EyeRenderPose[0].Position;
    ovrVector3f r = EyeRenderPose[1].Position;
    QVector3D le(l.x,l.y,l.z);
    QVector3D re(r.x,r.y,r.z);
    eyeCenter = 0.5*(le+re);
    std::cout << eyeCenter.x()<< " " << eyeCenter.y() << " " << eyeCenter.z() << std::endl;
}

void CGMainWindow::loadModel() {
    QString filename = QFileDialog::getOpenFileName(this, "Load model ...", QString(), "(*.stl *.dae *.DAE *.blend *.wrl *.off *.ply *.obj *.3ds *.3DS)" );

    ogl->loadModel(filename);
}

MyGLWidget::MyGLWidget(CGMainWindow *mainwindow,QWidget* parent ) : QOpenGLWidget (parent) {
    main = mainwindow;
}

void get_bounding_box_for_node (const aiScene* scene, const aiNode* nd, aiVector3D* min, aiVector3D* max, aiMatrix4x4* trafo) {
    std::cout << "(" << nd->mName.C_Str();
    aiMatrix4x4 prev = *trafo;
    aiMultiplyMatrix4(trafo,&nd->mTransformation);

    for (unsigned int n= 0; n < nd->mNumMeshes; n++) {
        const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
        for (unsigned int t = 0; t < mesh->mNumVertices; t++) {
            aiVector3D tmp = mesh->mVertices[t];
            aiTransformVecByMatrix4(&tmp,trafo);

            min->x = std::min(min->x,tmp.x);
            min->y = std::min(min->y,tmp.y);
            min->z = std::min(min->z,tmp.z);

            max->x = std::max(max->x,tmp.x);
            max->y = std::max(max->y,tmp.y);
            max->z = std::max(max->z,tmp.z);
        }
    }

    for (unsigned int n = 0; n < nd->mNumChildren; n++)
        get_bounding_box_for_node(scene,nd->mChildren[n],min,max,trafo);

    *trafo = prev;
    std::cout << ")";
}

void get_bounding_box (const aiScene *scene, aiVector3D* min, aiVector3D* max) {
	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z =  1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene,scene->mRootNode,min,max,&trafo);
}

void MyGLWidget::loadModel(QString filename) {

    const aiScene* scene = aiImportFile(filename.toLatin1(),aiProcessPreset_TargetRealtime_MaxQuality);

    if (scene == NULL) return;

//  std::cout << "NumAnimations = " << scene->mNumAnimations << std::endl;
//  std::cout << "NumCameras    = " << scene->mNumCameras << std::endl;
//  std::cout << "NumLights     = " << scene->mNumLights << std::endl;
//  std::cout << "NumMaterials  = " << scene->mNumMaterials << std::endl;
    std::cout << "NumMeshes     = " << scene->mNumMeshes << std::endl;
//  std::cout << "NumTextures   = " << scene->mNumTextures << std::endl;

    std::vector<QOpenGLTexture*> texture;

    for(unsigned int i=0;i<scene->mNumMaterials;i++) {
//      std::cout << "material = " << i << std::endl;
        aiMaterial* mtl = scene->mMaterials[i];
        aiColor4D color;
        mtl->Get(AI_MATKEY_COLOR_DIFFUSE,color);
//      std::cout << "color = " << color.r << " " << color.g << " " << color.b << " " << color.a << std::endl;
        QOpenGLTexture *tex = NULL;

        if (mtl->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            int texIndex = 0;
            aiString path;

            mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
//          std::cout << "tex = " << texIndex << " path = " << path.C_Str() << std::endl;

            QString str = filename.section('/',0,-2);
            QString texname(path.C_Str());
            texname = texname.replace('\\','/');
            str.append('/').append(texname);
//          std::cout << "fname = " << str.toStdString() << std::endl;

            QImage image(str);
            if (!image.isNull()) {
                tex = new QOpenGLTexture(image.mirrored());

                tex->setMinificationFilter(QOpenGLTexture::Linear);
                tex->setMagnificationFilter(QOpenGLTexture::Linear);
                // std::cout << tex->format() << std::endl;
            }
        }

        texture.push_back(tex);
    }

    scenes.push_back(scene);
    textures.push_back(texture);

    initMeshVBOandIBO(scene);

    aiVector3D scene_min, scene_max;

    if (scene)
        get_bounding_box(scene,&scene_min,&scene_max);

    bbMin = QVector3D(scene_min.x,scene_min.y,scene_min.z);
    bbMax = QVector3D(scene_max.x,scene_max.y,scene_max.z);

    QVector3D extent = bbMax - bbMin;

    QVector3D aaMin(191.518,-6.5818,2.9853);
    QVector3D aaMax(250.3,58.6074,55.9195);

    if (fabs(aaMax.x()+aaMin.x()-bbMax.x()-bbMin.x()) < aaMax.x()-aaMin.x()+bbMax.x()-bbMin.x()
    &&  fabs(aaMax.y()+aaMin.y()-bbMax.y()-bbMin.y()) < aaMax.y()-aaMin.y()+bbMax.y()-bbMin.y() )
        std::cout << "b = [" << bbMin.x() << "," << bbMax.x() << "] x [ "
                             << bbMin.y() << "," << bbMax.y() << "] x [ "
                             << bbMin.z() << "," << bbMax.z() << "]" << std::endl;
//  }

//  std::cout << "e = " << extent.x() << " " << extent.y() << " " << extent.z() << std::endl;
    zoom = 1.5/std::max(std::max(extent.x(),extent.y()),extent.z());
    center = (bbMin+bbMax)/2;
}


void CGMainWindow::loadHin(){



    QString fn = QFileDialog::getOpenFileName(this, "Load Ice Cube Data ...", QString(), "HIN files (*.hin)" );

    if (fn.isEmpty()) return;

    std::cout<<"Loading Ice Cube Data"<<std::endl;



    std::ifstream instream(fn.toLatin1(),std::ios::binary);

    //put all the lines of the file into 'lines'

    std::vector<std::string> lines;

    if(instream.is_open()){

        while(!instream.eof()){

            std::string st;

            getline(instream,st);

            lines.push_back(st);

        }

    }

    //std::cout<<"molecule1 \n";

    //find the start and end of a molecule

    std::vector<int> begin;

    std::vector<int> end;

    for(int i = 0; i< lines.size(); i++){

        //std::cout<<"molecule \n";

        std::string temp = lines[i];

        std::cout << temp << "\n";

        if(temp.substr(0,3) == "mol"){

            begin.push_back(i);

            //molecule m{temp};

            //ogl->struc.mols.push_back(m);

        }

        if(temp.substr(0,6) == "endmol"){

            end.push_back(i);

        }

    }

    std::cout<<"begin: "<<begin.size()<<"\n";

     //find out where all the residues of one molecule start and end

    for(int monster=0;monster<begin.size();monster++){

        std::vector<int> beginr;//a vector storing all starts of the residues

        std::vector<int> endr;

        std::cout<<"monster:"<<monster<<"\n";

        for(int j = begin[monster]; j < end[monster]; j++){



            std::string temp = lines[j];

            if(temp.substr(0,3)=="res"){

                beginr.push_back(j);



            }

            if(temp.substr(0,6)=="endres"){

                endr.push_back(j);

            }

        }

        //std::cout<<"beginr.size: "<<beginr.size()<<"\n";

        //std::cout<<"end[monster]: "<<end[monster]<<"\n";

        molecule m;

        std::vector<QVector3D> posi;

        for(int dracula=0;dracula<beginr.size();dracula++){

                std::cout<<"reached residue \n";

                residue r;

                r.rname=lines[beginr[dracula]];

                std::cout<<r.rname<<"\n";

                //std::cout<<"beginr[dracula]: "<<beginr[dracula]<<"\n";

                //std::cout<<"endr[dracula]: "<<endr[dracula]<<"\n";

                for(int fumanchu =beginr[dracula];fumanchu<endr[dracula];fumanchu++){

                    std::string temp=lines[fumanchu];

                    if(temp.substr(0,4) == "atom"){

                       float x,y,z;

                       std::string name;

                       std::istringstream f(temp);

                       std::string str;

                       int len,ind;

                       std::string dummy;

                       int counter =0;

                       std::vector<std::pair<int,std::string>> seanconnery;



                       //splitting one line

                       while (getline(f, str, ' ')) {

                           if(counter==1){

                               std::stringstream sstr;

                               sstr << str;

                               sstr>>ind;

                           }



                           if(counter==3){

                               std::stringstream sstr;

                               sstr << str;

                               name=str;

                           }

                           if(counter==7){

                               std::stringstream sstr;

                               sstr << str;

                               sstr >> x;

                           }

                           if(counter==8){

                               std::stringstream sstr;

                               sstr << str;

                               sstr >> y;

                           }

                           if(counter==9){

                               std::stringstream sstr;

                               sstr << str;

                               sstr >> z;

                           }

                           /*if(counter==10){

                               std::stringstream sstr;

                               sstr<< str;

                               sstr>>len;

                           }*/

                           if(counter>10&&counter%2==1){

                               std::stringstream sstr;

                               sstr<<str;

                               sstr>>len;

                           }

                           if(counter>10&&counter%2==0){

                               std::stringstream sstr;

                               sstr<<str;

                               sstr>>dummy;

                               seanconnery.push_back({len,dummy});

                               //std::cout<<seanconnery[0].first<<"\n";

                           }

                           counter++;



                       }

                       QVector3D hans {x,y,z};

                       posi.push_back(hans);

                       //std::cout<<hans.x()<<", "<<hans.y()<<", "<<hans.z()<<"\n";

                       atom a{ind,name,hans,0.0,seanconnery,QVector3D{0,0,0}};

                       std::cout<<"yesss";

                       r.elements.push_back(a);



                    }

                }

              m.parts.push_back(r);



             }

        m.positions=posi;
        float xMax=-std::numeric_limits<float>::infinity();
        float xMin=std::numeric_limits<float>::infinity();
        float yMax=-std::numeric_limits<float>::infinity();
        float yMin=std::numeric_limits<float>::infinity();
        float zMax=-std::numeric_limits<float>::infinity();
        float zMin=std::numeric_limits<float>::infinity();
        for(int j=0;j<m.parts.size();j++){
            for(int k=0;k<m.parts[j].elements.size();k++){
                QVector3D posi=m.parts[j].elements[k].pos;
                if(posi.x()>xMax){
                    xMax=posi.x();
                }
                if(posi.x()<xMin){
                    xMin=posi.x();
                }
                if(posi.y()>yMax){
                    yMax=posi.y();
                }
                if(posi.y()<yMin){
                    yMin=posi.y();
                }
                if(posi.z()>zMax){
                    zMax=posi.z();
                }
                if(posi.z()<zMin){
                    zMin=posi.z();
                }
            }

        }
float centralx=xMin+((xMax-xMin)/2);
float centraly=yMin+((yMax-yMin)/2);
float centralz=zMin+((zMax-zMin)/2);
ogl->central.push_back(QVector3D{centralx,centraly,centralz});
ogl->struc.mols.push_back(m);
ogl->phiMol.push_back(0.0f);
ogl->thetaMol.push_back(0.0f);
ogl->xPos.push_back(0.0f);
ogl->yPos.push_back(0.0f);


for(int h=0;h<ogl->struc.mols.size();h++){
    //std::cout<<"h = "<<h<<std::endl;
    ogl->struc.mols[h].boundingBox = ogl->calculateBoundingBox(ogl->struc.mols[h].positions,1.0f);
}

    }



    QString fn2 = QFileDialog::getOpenFileName(this, "Load Ice Cube Data ...", QString(), "txt files (*.txt)" );

    if (fn2.isEmpty()) return;

    std::cout<<"Loading Radius"<<std::endl;



    std::ifstream ist(fn2.toLatin1(),std::ios::binary);

    std::vector<std::tuple<std::string,float,QVector3D>> vec;

    int count=0;

    if(ist.is_open()){

       while(!ist.eof()){

           float value;

           std::string key;

           float r,g,b;

           ist>>key>>value;

           ist >>r>>g>>b;

           QVector3D v {r/255,g/255,b/255};

           std::cout<<key<<", "<<value<<"\n";

           std::tuple<std::string,float,QVector3D> ma{key,value,v};

           vec.push_back(ma);

           std::cout<<count++<<"\n";

       }

    }

    else{

        std::cout<<"File not found \n";

    }

    std::cout<<"change atom size"<<"\n";

    for(int h=0;h<ogl->struc.mols.size();h++){

        for(int i=0;i<ogl->struc.mols[h].parts.size();i++){

            for(int j=0;j<ogl->struc.mols[h].parts[i].elements.size();j++){

                for(int k=0;k<vec.size();k++){

                    //std::cout<<"h:"<<h<<" i: "<<i<<",j: "<<j<<" k:"<<k<<"\n";

                    std::string nam=ogl->struc.mols[h].parts[i].elements[j].name;

                    if(nam==std::get<0>(vec[k])){

                        ogl->struc.mols[h].parts[i].elements[j].radius=std::get<1>(vec[k]);

                        ogl->struc.mols[h].parts[i].elements[j].color=std::get<2>(vec[k]);

                    }

                }

            }

        }

    }


    std::cout<<"finished loading"<<"\n";

    ogl->flag = true;

    ogl->update();
    //std::cout<<"Bounding Box size = "<<ogl->struc.mols[0].boundingBox.size()<<std::endl;
}

void MyGLWidget::updateBoundingBox(molecule mol){

        for(int j=0;j<mol.boundingBox.size();j++){
            QVector4D b= QVector4D(mol.boundingBox[j],1.0);
            b = modelView*b;
            mol.boundingBox[j] = QVector3D(b.x(),b.y(),b.z());
            //mol.boundingBox[j] = QVector3D(10,10,10);

        }

}

std::vector<QVector3D> MyGLWidget::calculateBoundingBox(std::vector<QVector3D> koords,float scale){
    float xmax,xmin,ymax,ymin,zmax,zmin;

    xmin = koords[0].x();
    xmax = xmin;
    ymin = koords[0].y();
    ymax = ymin;
    zmin = koords[0].z();
    zmax = zmin;

    for(int i=0;i<koords.size();i++){
        float x = koords[i].x();
        float y = koords[i].y();
        float z = koords[i].z();

        if(x<xmin)
            xmin = x;
        if(x>xmax)
            xmax = x;
        if(y<ymin)
            ymin = y;
        if(y>xmax)
            ymax = y;
        if(z<xmin)
            zmin = z;
        if(z>xmax)
            zmax = z;
    }

    std::vector<QVector3D> box;
    box.push_back(QVector3D(xmin,ymin,zmin));
    box.push_back(QVector3D(xmin,ymin,zmax));
    box.push_back(QVector3D(xmin,ymax,zmin));
    box.push_back(QVector3D(xmin,ymax,zmax));
    box.push_back(QVector3D(xmax,ymin,zmin));
    box.push_back(QVector3D(xmax,ymin,zmax));
    box.push_back(QVector3D(xmax,ymax,zmin));
    box.push_back(QVector3D(xmax,ymax,zmax));


    return box;


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
    std::cout << "InitSolidSphereVBO" << std::endl;
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

void MyGLWidget::initCylinderVBO(){

    std::vector<QVector3D> ico;

    int n=100;

    for(int i=0; i<=n; i++){

        float theta = (float)i* (2*M_PI/n);

        // Vertex
        float vertX = cos(theta);
        float vertY = sin(theta);

        ico.push_back(QVector3D(vertX,vertY,0));
        ico.push_back(QVector3D(vertX,vertY,1));


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

    vboCylinderId = id;
    vboCylinderSize = static_cast<int>(ico.size());
}

void MyGLWidget::initCubeVBOandIBO() {
    std::vector<QVector3D> vboCube;

    vboCube.push_back(QVector3D(-1,-1,-1));
    vboCube.push_back(QVector3D( 1,-1,-1));
    vboCube.push_back(QVector3D(-1, 1,-1));
    vboCube.push_back(QVector3D( 1, 1,-1));
    vboCube.push_back(QVector3D(-1,-1, 1));
    vboCube.push_back(QVector3D( 1,-1, 1));
    vboCube.push_back(QVector3D(-1, 1, 1));
    vboCube.push_back(QVector3D( 1, 1, 1));

    // unsigned int indices[] = { 0,1,2,1,3,2,4,6,5,6,7,5,0,4,5,0,5,1,2,7,6,2,3,7,7,3,1,7,1,5,0,2,6,0,6,4 };
    // std::vector<unsigned int> iboCube(indices+0,indices+36);

    std::vector<unsigned int> iboCube { 0,1,2,1,3,2,4,6,5,6,7,5,0,4,5,0,5,1,2,7,6,2,3,7,7,3,1,7,1,5,0,2,6,0,6,4 };

    glGenBuffers(1,&vboCubeId);

    glBindBuffer(GL_ARRAY_BUFFER,vboCubeId);
    glBufferData(GL_ARRAY_BUFFER,vboCube.size()*sizeof(QVector3D),vboCube.data(),GL_STATIC_DRAW);

    vboCubeSize = (unsigned int) vboCube.size();

    glGenBuffers(1,&iboCubeId);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,iboCubeId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,iboCube.size()*sizeof(unsigned int),iboCube.data(),GL_STATIC_DRAW);

    iboCubeSize = (unsigned int) iboCube.size();
}

void MyGLWidget::initCubeMap(QString name) {
    const QImage posx = QImage(QString("_positive_x.jpg").prepend(name)).convertToFormat(QImage::Format_RGBA8888);
    const QImage posy = QImage(QString("_positive_y.jpg").prepend(name)).convertToFormat(QImage::Format_RGBA8888);
    const QImage posz = QImage(QString("_positive_z.jpg").prepend(name)).convertToFormat(QImage::Format_RGBA8888);
    const QImage negx = QImage(QString("_negative_x.jpg").prepend(name)).convertToFormat(QImage::Format_RGBA8888);
    const QImage negy = QImage(QString("_negative_y.jpg").prepend(name)).convertToFormat(QImage::Format_RGBA8888);
    const QImage negz = QImage(QString("_negative_z.jpg").prepend(name)).convertToFormat(QImage::Format_RGBA8888);

    cubemap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    cubemap->create();
    cubemap->setSize(posx.width(), posx.height(), posx.depth());
    cubemap->setFormat(QOpenGLTexture::RGBA8_UNorm);
    cubemap->allocateStorage();
    cubemap->setData(0, 0, QOpenGLTexture::CubeMapPositiveX,
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            (const void*)posx.constBits(), 0);
    cubemap->setData(0, 0, QOpenGLTexture::CubeMapPositiveY,
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            (const void*)posy.constBits(), 0);
    cubemap->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ,
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            (const void*)posz.constBits(), 0);
    cubemap->setData(0, 0, QOpenGLTexture::CubeMapNegativeX,
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            (const void*)negx.constBits(), 0);
    cubemap->setData(0, 0, QOpenGLTexture::CubeMapNegativeY,
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            (const void*)negy.constBits(), 0);
    cubemap->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ,
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            (const void*)negz.constBits(), 0);
    cubemap->generateMipMaps();
    cubemap->setWrapMode(QOpenGLTexture::ClampToEdge);
    cubemap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    cubemap->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

}

void MyGLWidget::initCrosshairVBO() {
    std::vector<QVector3D> ico;

    ico.push_back(QVector3D(0,0,0));
    ico.push_back(QVector3D(1,0,0));

    int n=100;
    for(int i=0; i<=n; i++){

        float theta = (float)i* (2*M_PI/n);
        // Vertex
        float vertX = cos(theta);
        float vertY = sin(theta);

        ico.push_back(QVector3D(vertX,vertY,0));
    }
    //ico.push_back(QVector3D(1,0,0));


    GLuint id;
    glGenBuffers(1,&id);

    glBindBuffer(GL_ARRAY_BUFFER,id);
    glBufferData(GL_ARRAY_BUFFER,ico.size()*sizeof(QVector3D),ico.data(),GL_STATIC_DRAW);

    vboCrosshairId = id;
    vboCrosshairSize = static_cast<int>(ico.size());
}


void MyGLWidget::initShaders() {
    setlocale(LC_NUMERIC,"C");
    /*
    if (!p_Diffuse.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/VertexShaderDiffuse.glsl"))
        close();

    if (!p_Diffuse.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FragmentShaderDiffuse.glsl"))
        close();

    if (!p_Diffuse.link())
        close();

    if (!p_Diffuse.bind())
        close();
    */
    if (!p_Color.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/VertexShaderColor.glsl"))
        close();

    if (!p_Color.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FragmentShaderColor.glsl"))
        close();

    if (!p_Color.link())
        close();

    if (!p_Color.bind())
        close();


    if (!p_Cube.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/VertexShaderEnvMap.glsl"))
        close();

    if (!p_Cube.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FragmentShaderEnvMap.glsl"))
        close();

    if (!p_Cube.link())
        close();

    if (!p_Cube.bind())
        close();

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

void MyGLWidget::initMeshVBOandIBO(const aiScene* scene) {
    std::vector<unsigned int> vboId, vboSize, iboId, iboSize;

    for(unsigned int k=0;k<scene->mNumMeshes;k++) {
        const aiMesh* mesh = scene->mMeshes[k];

        std::vector<float> vtn;
        std::vector<unsigned short> ifs;

//      std::cout << mesh->mNumVertices << std::endl;

        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            if (mesh->HasPositions()) {
                vtn.push_back(mesh->mVertices[i].x);
                vtn.push_back(mesh->mVertices[i].y);
                vtn.push_back(mesh->mVertices[i].z);
            }

            if (mesh->HasTextureCoords(0)) {
                vtn.push_back(mesh->mTextureCoords[0][i].x);
                vtn.push_back(mesh->mTextureCoords[0][i].y);
            }

            if (mesh->HasNormals()) {
                vtn.push_back(mesh->mNormals[i].x);
                vtn.push_back(mesh->mNormals[i].y);
                vtn.push_back(mesh->mNormals[i].z);
            }
        }

        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace* face = &mesh->mFaces[i];

            if (face->mNumIndices == 3) {
                for(unsigned int j = 0; j < face->mNumIndices; j++)
                    ifs.push_back((unsigned short) face->mIndices[j]);
            } else
                std::cout << "mesh not triangulated" << std::endl;
        }

        int sizePerVertex = 0;
        if (mesh->HasPositions()) sizePerVertex += 3;
        if (mesh->HasTextureCoords(0)) sizePerVertex += 2;
        if (mesh->HasNormals()) sizePerVertex += 3;

        GLuint id;
        glGenBuffers(1,&id);

        glBindBuffer(GL_ARRAY_BUFFER,id);
        glBufferData(GL_ARRAY_BUFFER,vtn.size()*sizeof(float),vtn.data(),GL_STATIC_DRAW);

        vboId.push_back(id);
        vboSize.push_back(vtn.size()/sizePerVertex);

        glGenBuffers(1,&id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,ifs.size()*sizeof(unsigned short),ifs.data(),GL_STATIC_DRAW);

        iboId.push_back(id);
        iboSize.push_back(ifs.size());
    }

    vboTrianglesId.push_back(vboId);
    vboTrianglesSize.push_back(vboSize);
    iboTrianglesId.push_back(iboId);
    iboTrianglesSize.push_back(iboSize);
}

void MyGLWidget::recursive_render(int sceneId,const aiNode* node,QMatrix4x4 M) {
    aiMatrix4x4 trafo = node->mTransformation;
    QMatrix4x4 T(trafo.a1,trafo.a2,trafo.a3,trafo.a4,
                 trafo.b1,trafo.b2,trafo.b3,trafo.b4,
                 trafo.c1,trafo.c2,trafo.c3,trafo.c4,
                 trafo.d1,trafo.d2,trafo.d3,trafo.d4);
    M = M*T;

    for (unsigned int n = 0;n < node->mNumMeshes; n++) {
        unsigned int meshId = node->mMeshes[n];
        float hasTex = 0.0f;
        unsigned int sizePerVertex = 6;

        aiMesh* mesh = scenes[sceneId]->mMeshes[meshId];
        if (mesh->HasTextureCoords(0)) {
            if (textures[sceneId][mesh->mMaterialIndex] != NULL) {
                textures[sceneId][mesh->mMaterialIndex]->bind(0);
                hasTex = 1.0f;
                sizePerVertex += 2;
            }
        }

        // aiColor4D color;
        // scenes[sceneId]->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE,color);
        // QVector4D diffuse(color.r,color.g,color.b,color.a);

        p_Diffuse.bind();
        p_Diffuse.setUniformValue("uPMat", projection);
        p_Diffuse.setUniformValue("uMVMat", M);
        p_Diffuse.setUniformValue("uNMat", M.normalMatrix());
        p_Diffuse.setUniformValue("uHasTex", hasTex);
        // p_Diffuse.setUniformValue("uDiffuse", diffuse);
        p_Diffuse.setUniformValue("texture", 0);

        glBindBuffer(GL_ARRAY_BUFFER, vboTrianglesId[sceneId][meshId]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,iboTrianglesId[sceneId][meshId]);

        int vertexLocation = p_Diffuse.attributeLocation("a_position");
        p_Diffuse.enableAttributeArray(vertexLocation);
        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizePerVertex*sizeof(float), 0);

        if (hasTex != 0.0f) {
            int texCoordLocation = p_Diffuse.attributeLocation("a_tex");
            p_Diffuse.enableAttributeArray(texCoordLocation);
            glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, sizePerVertex*sizeof(float), (const GLvoid*) (3*sizeof(float)));
        }

        int normalLocation = p_Diffuse.attributeLocation("a_normal");
        p_Diffuse.enableAttributeArray(normalLocation);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, sizePerVertex*sizeof(float), (const GLvoid*) ((sizePerVertex-3)*sizeof(float)));

        glDrawElements(GL_TRIANGLES,iboTrianglesSize[sceneId][meshId],GL_UNSIGNED_SHORT,0);

    }

    for (unsigned int n = 0; n < node->mNumChildren; n++)
        recursive_render(sceneId, node->mChildren[n],M);
}

void MyGLWidget::initializeOVR() {
    ovrResult result = ovr_Initialize(NULL);
    std::cout << "ovr_Initialize = " << OVR_SUCCESS(result) << std::endl;

    ovrGraphicsLuid luid;
    result = ovr_Create(&session, &luid);
    std::cout << "ovr_Create = " << OVR_SUCCESS(result) << std::endl;

    hmdDesc = ovr_GetHmdDesc(session);
    std::cout << "hmd resolution = " << hmdDesc.Resolution.w << " " << hmdDesc.Resolution.h << std::endl;
    // ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };
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

    //ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_EyeLevel);
}

void MyGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    initShaders();
    initCylinderVBO();
    initSolidSphereVBO();
    initCrosshairVBO();
    initializeOVR();
    initCubeVBOandIBO();
    initCubeMap(QString(":/Maps/Lobby"));

    glClearColor(0.6,0.6,0.6,1);

    glEnable(GL_DEPTH_TEST);


    bbMin[0] = bbMin[1] = bbMin[2] =  std::numeric_limits<double>::infinity();
    bbMax[0] = bbMax[1] = bbMax[2] = -std::numeric_limits<double>::infinity();
    center[0] = center[1] = center[2] = 0.0;
    zoom = 1.0;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(10);

    icke.setToIdentity();
    crosshairView.setToIdentity();
    crosshairView.translate(0,0,-0.2);
    crosshairView.scale(0.001f);
}

void MyGLWidget::drawCylinder(const QVector3D& c1, const QVector3D& c2, float r,const QVector3D color,QMatrix4x4 molView) {


    QVector3D d=c1-c2;
    QVector3D a(0,0,1);
    QVector3D rot = QVector3D::crossProduct(d,a);
    float phi = asin(rot.length()/(a.length()*d.length()))*180/M_PI;
    if(QVector3D::dotProduct(d,a)>0)
        phi = 180-phi;

    QMatrix4x4 M(molView);
    M.translate(c1);
    M.rotate(phi,rot.normalized());
    M.scale(r,r,(d).length());

    //const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uMVMat", M);
    p_Phong.setUniformValue("uNMat", M.normalMatrix());
    /*
    p_Phong.setUniformValue("uAmbient",QVector3D(material[t][0],material[t][1],material[t][2]));
    p_Phong.setUniformValue("uDiffuse",QVector3D(material[t][3],material[t][4],material[t][5]));
    p_Phong.setUniformValue("uSpecular",QVector3D(material[t][6],material[t][7],material[t][8]));
    p_Phong.setUniformValue("uShininess",material[t][9]);
    */
    //p_Phong.setUniformValue("uDiffuse",color);

    p_Phong.setUniformValue("uAmbient",color);
    p_Phong.setUniformValue("uDiffuse",QVector3D(0.3,0.4,0.4));
    p_Phong.setUniformValue("uSpecular",QVector3D(0.3,0.4,0.4));
    p_Phong.setUniformValue("uShininess",0.3f);

    glBindBuffer(GL_ARRAY_BUFFER, vboCylinderId);
    int vertexLocation = p_Phong.attributeLocation("a_position");
    p_Phong.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    int normalLocation = p_Phong.attributeLocation("a_normal");
    p_Phong.enableAttributeArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_TRIANGLE_STRIP,0,vboCylinderSize);
}

void MyGLWidget::drawSolidSphere(const QVector3D& c, float r,const QVector3D color,QMatrix4x4 molView) {
    QMatrix4x4 M(molView);
    M.translate(c);
    M.scale(r);

   // const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uMVMat", M);
    p_Phong.setUniformValue("uNMat", M.normalMatrix());
    p_Phong.setUniformValue("uPMat", this->projection);
    /*
    p_Phong.setUniformValue("uAmbient",QVector3D(material[t][0],material[t][1],material[t][2]));
    p_Phong.setUniformValue("uDiffuse",QVector3D(material[t][3],material[t][4],material[t][5]));
    p_Phong.setUniformValue("uSpecular",QVector3D(material[t][6],material[t][7],material[t][8]));
    p_Phong.setUniformValue("uShininess",material[t][9]);
    */
    //p_Phong.setUniformValue("uDiffuse",color);

    p_Phong.setUniformValue("uAmbient",color*0.7);

    p_Phong.setUniformValue("uDiffuse",color*0.8);
    p_Phong.setUniformValue("uSpecular",QVector3D(1,1,1));
    p_Phong.setUniformValue("uShininess",1.2f);

    glBindBuffer(GL_ARRAY_BUFFER, vboSolidSphereId);
    int vertexLocation = p_Phong.attributeLocation("a_position");
    p_Phong.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    int normalLocation = p_Phong.attributeLocation("a_normal");
    p_Phong.enableAttributeArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_TRIANGLES,0,vboSolidSphereSize);
}

void MyGLWidget::drawBoundingBox(std::vector<QVector3D> box,QMatrix4x4 molView,QVector4D color) {
    QMatrix4x4 M(molView);
    QVector3D c;
    float sx,sy,sz;

    //std::cout<<"Bounding Box size = "<<struc.mols[0].boundingBox.size()<<std::endl;

    c = (box[0]+box[7])/2;
    sx = (box[0].x()-box[7].x())/2;
    sy = (box[0].y()-box[7].y())/2;
    sz = (box[0].z()-box[7].z())/2;

    //std::cout<<"boxsize"<<box[0].z()<<std::endl;
    //c = QVector3D(0,0,0);
    //sx,sy,sz = 1;
    //sx = 1.0f;
    //sy = 1.0f;
    //sz = 1.0f;


    M.translate(c);
    M.scale(sx,sy,sz);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

/*
    float aspect = (float) width()/height();
    projection.setToIdentity();
    projection.ortho(-aspect,aspect,-1.0,1.0,-10.0,10.0);
    modelView = RNow;
    modelView.scale(zoom,zoom,zoom);
    //modelView.scale(10,10,10);
    modelView.translate(-center);
*/

    p_Color.bind();
    p_Color.setUniformValue("uMVMat", M);
    p_Color.setUniformValue("uNMat", M.normalMatrix());
    p_Color.setUniformValue("uPMat", this->projection);
    p_Color.setUniformValue("uDiffuse",QVector3D(color.x(),color.y(),color.z()));

    glBindBuffer(GL_ARRAY_BUFFER,vboCubeId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,iboCubeId);

    int vertexLocation = p_Color.attributeLocation("a_position");
    p_Color.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), 0);

    glDrawElements(GL_TRIANGLES,iboCubeSize,GL_UNSIGNED_INT,0);


}


void MyGLWidget::drawCrosshair() {
    QMatrix4x4 M(modelView);
    //M.translate(c);
    M.scale(5);

    p_Color.bind();
    p_Color.setUniformValue("uMVMat", crosshairView);
    p_Color.setUniformValue("uNMat", crosshairView.normalMatrix());
    p_Color.setUniformValue("uPMat", this->projection);
    p_Color.setUniformValue("uDiffuse",QVector3D(1,1,1));


    glBindBuffer(GL_ARRAY_BUFFER, vboCrosshairId);
    int vertexLocation = p_Phong.attributeLocation("a_position");
    p_Color.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    int normalLocation = p_Phong.attributeLocation("a_normal");
    p_Color.enableAttributeArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_TRIANGLE_FAN,0,vboCrosshairSize);
}


void MyGLWidget::drawMolecule(molecule mol,QMatrix4x4 molView,bool selected){

    for(int i=0;i<mol.parts.size();i++){
        for(int j=0;j<mol.parts[i].elements.size();j++){
            drawSolidSphere(mol.parts.at(i).elements[j].pos,mol.parts.at(i).elements[j].radius/200,mol.parts.at(i).elements[j].color,molView);
        }
    }
    for(int i=0;i<mol.parts.size();i++){
        for(int j=0;j<mol.parts[i].elements.size();j++){
            QVector3D pos1 = mol.parts[i].elements[j].pos;
            for(int k=0;k<mol.parts[i].elements[j].bonds.size();k++){
                int iBond = mol.parts[i].elements[j].bonds[k].first;
               // std::cout<<iBond<<"\n";
                QVector3D pos2;
                pos2=mol.positions[(iBond-1)];
                drawCylinder(pos1,pos2,0.1,QVector3D(0.1,0.1,0.1),molView);
            }
        }
    }

    if(showBoundingBox){
        drawBoundingBox(mol.boundingBox,molView,QVector4D(0.0f,0.0f,0.7f,1.0f));
    }
/*
    if(selected){
        drawBoundingBox(mol.boundingBox,molView,QVector4D(0.7f,0.0f,0.0f,1.0f));
    }
*/
}

void MyGLWidget::drawCubeMap(){

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

/*
    float aspect = (float) width()/height();
    projection.setToIdentity();
    projection.ortho(-aspect,aspect,-1.0,1.0,-10.0,10.0);
    modelView = RNow;
    modelView.scale(zoom,zoom,zoom);
    //modelView.scale(10,10,10);
    modelView.translate(-center);
*/
    cubemap->bind(0);

    p_Cube.bind();
    p_Cube.setUniformValue("uPMat", projection);
    p_Cube.setUniformValue("uMVMat", modelView);
    p_Cube.setUniformValue("cubemap", 0);

    glBindBuffer(GL_ARRAY_BUFFER,vboCubeId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,iboCubeId);

    int vertexLocation = p_Cube.attributeLocation("a_position");
    p_Cube.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), 0);

    glDrawElements(GL_TRIANGLES,iboCubeSize,GL_UNSIGNED_INT,0);


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

/*
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

    std::cout << "imin = " << imin << "  jmin = " << jmin << "  lmin = " << lmin << std::endl;

    return lmin;
}
*/

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
void MyGLWidget::pressA(){
    if(selectIndex<struc.mols.size())
        selectIndex++;
    std::cout<<selectIndex<<"\n";
    //pickMolecule();
    update();
}
void MyGLWidget::pressB(){
    if(selectIndex>=struc.mols.size()-1)
         selectIndex--;
     std::cout<<selectIndex<<"\n";
    update();
}

void MyGLWidget::paintGL() {
    //if (vboTrianglesId.size() == 0) return


    if (viewMode == 1) {
        float aspect = (float) width()/height();
        projection.setToIdentity();
        projection.ortho(-aspect,aspect,-1.0,1.0,-10.0,10.0);
        //projection.perspective(45,aspect,1,200);
        //modelView = RNow;
        modelView.setToIdentity();
        modelView.rotate(theta,1.0,0.0,0.0);
        modelView.rotate(phi,0.0,1.0,0.0);
        modelView.scale(zoom,zoom,zoom);
        modelView.translate(-center);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0,0,width(),height());
        for(unsigned int i=0;i<scenes.size();i++)
            recursive_render(i, scenes[i]->mRootNode,modelView);

        modelView.scale(100,100,100);
        glEnable(GL_CULL_FACE);
        drawCubeMap();
        glDisable(GL_CULL_FACE);

        modelView.scale(0.01,0.01,0.01);


        if (struc.mols.size()==0){
            drawSolidSphere(QVector3D(1,0,0),0.3,QVector3D(0.2,0.2,0.2),modelView);
            drawSolidSphere(QVector3D(0,0,0),0.3,QVector3D(0.2,0.2,0.2),modelView);
            drawCylinder(QVector3D(0,0,0),QVector3D(1,0,0),0.2,QVector3D(0.2,0.2,0.2),modelView);
         }
        if(struc.mols.size()!=0)
            for(int i=0;i<struc.mols.size();i++){
                struc.mols[i].molView = modelView;
                drawMolecule(struc.mols[i],struc.mols[i].molView,false);

            }

        //Crosshair
        drawCrosshair();



    }
    ovrVector2f ve;
    ovrVector2f vp;
    if (viewMode == 2) {
         ovrSessionStatus sessionStatus;
         ovr_GetSessionStatus(session, &sessionStatus);


         // Hand-Tracking
         uint ConnectedControllerTypes = ovr_GetConnectedControllerTypes(session);
         ovrQuatf qNow = {0,0,0,1};
         ovrInputState    inputState;
         // If controllers are connected, display their axis status.
         //if (ConnectedControllerTypes & (ovrControllerType_LTouch | ovrControllerType_RTouch)) {

           if (OVR_SUCCESS(ovr_GetInputState(session, ovrControllerType_XBox, &inputState))) {
             //std::cout<<"XBox angeschlossen \n";
             //std::cout<<"XBox angeschlossen"  << inputState.Buttons << "\n";



             if (buttonAPressed&!(inputState.Buttons & ovrButton_A))
                 {
               pressA();

             }
             buttonAPressed = inputState.Buttons & ovrButton_A;

             if(buttonBPressed&!(inputState.Buttons & ovrButton_B)){
                pressB();
             }
             buttonBPressed = inputState.Buttons & ovrButton_B;


             if (buttonXPressed&!(inputState.Buttons & ovrButton_X))
                 {
               showBoundingBox=!showBoundingBox;

             }
             buttonXPressed = inputState.Buttons & ovrButton_X;


             if (buttonYPressed&!(inputState.Buttons & ovrButton_Y))
                 {
               showCrossHair=!showCrossHair;

             }
             buttonYPressed = inputState.Buttons & ovrButton_Y;


             if(ovrButton_RThumb){

                 ve=inputState.Thumbstick[1];
                 //std::cout<<"x : "<<ve.x<<std::endl;
                 //std::cout<<"y : "<<ve.y<<std::endl;
                 if(struc.mols.size()!=0){
                     phiMol[selectIndex]+=ve.y;
                     thetaMol[selectIndex]+=ve.x;
                }

             }
             if(ovrButton_LThumb){
                 vp=inputState.Thumbstick[0];
                // std::cout<<inputState.Thumbstick[1].x;
                 //std::cout<<inputState.Thumbstick[1].y;
                 if(struc.mols.size()!=0){
                     xPos[selectIndex]+=(vp.x/2);
                     yPos[selectIndex]+=(vp.y/2);
                 }
             }
             if((inputState.Buttons&ovrButton_Up)){
                float s = 0.02;
                QVector3D t = s*sightz;
                icke.translate(t);
                //std::cout<<"up"<<std::endl;
             }
             if(inputState.Buttons&ovrButton_Down){
                 float s = -0.02;
                 QVector3D t = s*sightz;
                 icke.translate(t);
             }
             if(inputState.Buttons&ovrButton_Left){
                 float s = 0.02;
                 QVector3D t = s*sighty;
                 icke.translate(t);
             }
             if(inputState.Buttons&ovrButton_Right){
                 float s = -0.02;
                 QVector3D t = s*sighty;
                 icke.translate(t);
             }
             if(inputState.Buttons&ovrButton_RShoulder){
                 std::cout<<"RS \n";
             }



             double frameTiming = ovr_GetPredictedDisplayTime(session, frameIndex);
             ovrTrackingState trackState = ovr_GetTrackingState(session, frameTiming, ovrTrue);
             ovrPosef         handPoses[2];

            for (int hand = 0; hand < 2; ++hand) {
                 // Grab hand poses useful for rendering hand or controller representation
                 handPoses[hand]  = trackState.HandPoses[hand].ThePose;
                 /*std::cout << (hand==ovrHand_Left? "Left Hand Position:" : "Right Hand Position:")
                                                                  << " X = " << handPoses[hand].Position.x
                                                                  << " Y = " << handPoses[hand].Position.y
                                                                  << " Z = " << handPoses[hand].Position.z << std::endl;
                 std::cout << (hand==ovrHand_Left? "Left Hand Orientation:" : "Right Hand Orientation:")
                                                                  << " X = " << handPoses[hand].Orientation.x
                                                                  << " Y = " << handPoses[hand].Orientation.y
                                                                  << " Z = " << handPoses[hand].Orientation.z
                                                                  << " W = " << handPoses[hand].Orientation.w << std::endl;
                 */qNow = handPoses[hand].Orientation;
             }
         }

/*
         if (sessionStatus.IsVisible)
             std::cout << "visible" << std::endl;
         else
             std::cout << "not visible" << std::endl;
*/
         // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
         ovrEyeRenderDesc eyeRenderDesc[2];

         eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
         eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

         // Get eye poses, feeding in correct IPD offset
         ovrPosef    EyeRenderPose[2];
         ovrVector3f HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset, eyeRenderDesc[1].HmdToEyeOffset };
         for(int eye = 0; eye < 2; eye++) {
             ovrFovPort fov = hmdDesc.DefaultEyeFov[eye];

             //std::cout << "defaultEyeFov = " << fov.DownTan << " " << fov.UpTan << " " << fov.RightTan << " " << fov.LeftTan << std::endl;
             //std::cout << "HmdToEyeOffset[" << eye << "] = " << HmdToEyeOffset[eye].x << " " << HmdToEyeOffset[eye].y << " " << HmdToEyeOffset[eye].z << std::endl;
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

            ovrMatrix4f P = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 150.0f, ovrProjection_None);
            projection = QMatrix4x4(&P.M[0][0]);

            modelView.setToIdentity();
            ovrQuatf q = EyeRenderPose[eye].Orientation;
            modelView.rotate(QQuaternion(q.w,q.x,q.y,q.z).conjugate());
            ovrVector3f p = EyeRenderPose[eye].Position;
            //modelView.translate(-p.x,-p.y,-p.z);
            //modelView.rotate(QQuaternion(qNow.w, qNow.x, qNow.y, qNow.z));
            //modelView.scale(0.25,0.25,0.25);
           // modelView.scale(zoom,zoom,zoom);
            //modelView.rotate(-90,1,0,0);
            //modelView.translate(-center);


            //Berechnung der Sichtrichtung            
            QMatrix4x4 Mrot;
            Mrot.setToIdentity();
            Mrot.rotate(QQuaternion(q.w,q.x,q.y,q.z));

            QVector4D s;
            s = QVector4D(0,0,1,1);
            s = Mrot*s;
            s.normalize();
            sightz = QVector3D(s.x(),s.y(),s.z());
            sightz.normalize();

            s = QVector4D(1,0,0,1);
            s = Mrot*s;
            s.normalize();
            sighty = QVector3D(s.x(),s.y(),s.z());
            sighty.normalize();

            //Bewegung der Kamera
            modelView = modelView*icke;           
            // headPos = QVector3D(0,0,0)*modelView;
            //headPos = modelView*QVector3D(0,0,0);

            //Anfangs Orientierung

            modelView.translate(-p.x,-p.y,-p.z);
            modelView.scale(0.25,0.25,0.25);
            modelView.scale(zoom,zoom,zoom);
            modelView.rotate(-90,1,0,0);
            modelView.translate(-center);


            //Draw CubeMap

            modelView.scale(100,100,100);
            glEnable(GL_CULL_FACE);
            modelView.rotate(90,1,0,0);
            drawCubeMap();
            glDisable(GL_CULL_FACE);
            modelView.rotate(-90,1,0,0);
            modelView.scale(0.005,0.005,0.005);

            if (struc.mols.size()==0){
                drawSolidSphere(QVector3D(1,0,0),0.3,QVector3D(0.2,0.2,0.2),modelView);
                drawSolidSphere(QVector3D(0,0,0),0.3,QVector3D(0.2,0.2,0.2),modelView);
                drawCylinder(QVector3D(0,0,0),QVector3D(1,0,0),0.2,QVector3D(0.2,0.2,0.2),modelView);
             }

            //Draw molecules with rotation and position
            if(struc.mols.size()!=0)
                for(int i=0;i<struc.mols.size();i++){
                    QMatrix4x4 mat;
                    mat.setToIdentity();
                    //QVector t = xPos[i]*sightz+yPos[i];
                    mat.translate(xPos[i],0,0);
                    mat.translate(0,yPos[i],0);
                    mat.translate(central[i]);
                    mat.rotate(thetaMol[i],0,0,1);
                    mat.rotate(phiMol[i],0,1,0);
                    mat.translate(-central[i]);

                    struc.mols[i].molView = modelView*mat;
                    //modelView=modelView*mat;

                    bool selected =false;

                    if(selectIndex == i)
                        selected = true;

                    drawMolecule(struc.mols[i],struc.mols[i].molView,selected);
                    //updateBoundingBox(struc.mols[i]);
                    //modelView=modelView*mat.inverted();
                }

            //Crosshair
            if(showCrossHair)
                drawCrosshair();




            for(unsigned int i=0;i<scenes.size();i++)
                recursive_render(i, scenes[i]->mRootNode,modelView);

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
        //std::cout << "frameIndex = " << frameIndex << std::endl;
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
        //std::cout<<"ve"<<vp.x<<std::endl;
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

    double factor = (delta > 0)? 1.1 : 1/1.1;
    zoom *= factor;
    center = p + (center-p)/factor;

    update();
}

void MyGLWidget::pickMolecule(){
    double l;
    for(int i = 0;i<struc.mols.size();i++){
        for(int j=0;j<struc.mols[i].boundingBox.size()-2;j++){
            QVector3D a = struc.mols[i].boundingBox[j];
            QVector3D b = struc.mols[i].boundingBox[j+1];
            QVector3D c = struc.mols[i].boundingBox[j+2];

            a= projection*struc.mols[i].molView*a;
            b= projection*struc.mols[i].molView*b;
            c= projection*struc.mols[i].molView*c;

            QVector2D p(-1.0,1.0);
            QMatrix4x4 PM = projection*modelView;
            QVector4D u = PM.inverted()*QVector4D(p[0],p[1],-1.0,1.0);
            QVector4D v = PM.inverted()*QVector4D(p[0],p[1],1.0,1.0);
            QVector3D p0 = u.toVector3DAffine();
            QVector3D p1 = v.toVector3DAffine();

            l = intersectTriangle(p0,p1,a,b,c);
            std::cout<<"L = "<< l<<std::endl;
        }
    }

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
        phi -= (event->x()-mouseX)/5.0;
        theta -= (event->y()-mouseY)/5.0;
    }
    /*
    if (button == Qt::LeftButton) {
        QVector3D p1 = mouseToTrackball(mouseX,mouseY);
        QVector3D p2 = mouseToTrackball(event->x(),event->y());

      //phi += (event->x()-mouseX)/5.0;
      //theta += (event->y()-mouseY)/5.0;
      //if (theta < -90.0) theta = -90.0;
      //if (theta >  90.0) theta =  90.0;





//      QQuaternion q = trackball(p1,p2);
//      qNow = q * qNow;
//      qNow.normalize();

        //RNow = trackball(p1,p2) * RNow;

    }
*/
    if (button == Qt::RightButton) {
        QVector3D u0,u1,v0,v1;
        QVector3D vu,cu,p0,p1;
        pickLine(mouseX,mouseY,u0,v0);
        int model = -1,triangle = -1;
        vu = v0 - u0;
        double l = 0.0; // intersectTriangleSets(u0,v0,model,triangle);
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
