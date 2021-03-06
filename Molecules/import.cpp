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

#include "Molecules.h";
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
ogl->struc.mols.push_back(m);
    }

    QString fn2 = QFileDialog::getOpenFileName(this, "Load Ice Cube Data ...", QString(), "txt files (*.txt)" );
    if (fn2.isEmpty()) return;
    std::cout<<"Loading Radius"<<std::endl;
    //loading atomic radii and coloring the atoms using cpk coloring
    std::ifstream ist(fn2.toLatin1(),std::ios::binary);
    std::vector<std::tuple<std::string,float,QVector3D>> vec;
    int count=0;
    if(ist.is_open()){
        std::cout << "is_open0" << "\n";
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
    for (int i=0;i<ogl->struc.mols.size();i++){
       ogl->createLinesVBO(ogl->struc.mols[i]);
    }

    ogl->update();
}
