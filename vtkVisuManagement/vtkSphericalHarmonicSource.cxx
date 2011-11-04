//  Authors:	   Maxime Descoteaux, Jaime Garcia Guevara, Theodore Papadopoulo.
//
//  Description:  source of one spherical mesh.
//  Define the mesh and associated value for each direction
//  Deform the mesh after
//	
//   Copyright (c) 2007-2011, INRIA Sophia Antipolis, France, groups Odyssee, Athena.
//   Please see the copyright notice included in this distribution for full details.

#include <vector>
#include <complex>
#include <cmath>

#include <vtkSphericalHarmonicSource.h>

#include <vtkPolyDataNormals.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkSphereTesselator.h>


#include <boost/algorithm/minmax_element.hpp>
#include <boost/math/special_functions/legendre.hpp> //JGGBOOST
#include <tr1/cmath>

using namespace Visualization;
using Utils::direction;

static direction Cartesian2Spherical(direction v);

static matrix<double> ComputeSHMatrix (const int rank,vtkPolyData *shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections);
static matrix<double> ComputeSHMatrixMaxThesis(const int rank,vtkPolyData *shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections);

static matrix<double> ComputeSHmatrixTournier(const int rank,vtkPolyData *shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections);
static matrix<double> ComputeSHmatrixRshBasis(const int rank,vtkPolyData *shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections);


static void           TranslateAndDeformShell(vtkPolyData* shell,vtkPoints* outPts,double center[3],bool deform,vtkMatrix4x4* transform=0);

vtkCxxRevisionMacro(vtkSphericalHarmonicSource,"$Revision: 0 $");
vtkStandardNewMacro(vtkSphericalHarmonicSource);

vtkSphericalHarmonicSource::vtkSphericalHarmonicSource(int tess) {
    this->Radius = 0.5;
    this->Center[0] = 0.0;
    this->Center[1] = 0.0;
    this->Center[2] = 0.0;
    this->RotationMatrix = 0;

    this->SetNumberOfInputPorts(0);
    
    this->DeformOn();
    this->NormalizeOn();
    this->FlipXOff();
    this->FlipYOff();
    this->FlipZOn();  // By Default we flip the z-axis, the internal x,y,z have z flipped with respect to visu

    this->MaxThesisFuncOff();

    TesselationType = icosahedron;
    TesselationBasis = SHMatrix;
    Tesselation = tess;
    shell = 0;
    SphericalHarmonics = 0;
    Order = 4;
    this->SetNumberOfSphericalHarmonics (15);

#if 0
    shell = vtkPolyData::New(); 

    p_solid tesselationType ;

    switch (TesselationType)
    {
    case Cube : { tesselationType = cube ; break ; }
    case Dodecahedron : { tesselationType = dodecahedron ; break ; }
    case Icosahedron : { tesselationType = icosahedron ; break ; }
    case Octahedron : { tesselationType = octahedron ; break ; }
    case Tetrahedron : { tesselationType = tetrahedron ; break ; }
    }

    vtkSphereTesselator<double> sMesh(tesselationType);
    sMesh.tesselate(Tesselation);
    sMesh.getvtkTesselation(1,shell); 
    
    SphericalHarmonics = new double[NumberOfSphericalHarmonics];
    SphericalHarmonics[0] = 1.0;
    for(int i = 1; i < NumberOfSphericalHarmonics; i++)
        SphericalHarmonics[i] = 0.0;    
    Order = 4;//(int)(-3/2 + std::sqrt((float)(9/4 - 2*(1 - NumberOfSphericalHarmonics))));
    NumberOfSphericalHarmonics = (Order+1)*(Order+2)/2;


    matrix<double> PhiThetaDirection (shell->GetNumberOfPoints(),2);
    
    
    BasisFunction = ComputeSHMatrix2(NumberOfSphericalHarmonics,shell,
                                     FlipX,FlipY,FlipZ,PhiThetaDirection);
    
    PhiThetaShellDirections = PhiThetaDirection;
#endif
}

vtkSphericalHarmonicSource::~vtkSphericalHarmonicSource() {
    if (shell)
        shell->Delete();
    //    if (SphericalHarmonics)
    //        delete[] SphericalHarmonics;
    SphericalHarmonics = 0;
    shell = 0;
}

void
vtkSphericalHarmonicSource::SetNumberOfSphericalHarmonics(const int number) {

    NumberOfSphericalHarmonics = number;
    if (SphericalHarmonics)
        delete[] SphericalHarmonics;
    SphericalHarmonics = new double[NumberOfSphericalHarmonics];
    SphericalHarmonics[0] = 1.0;
    for(int i=1;i<NumberOfSphericalHarmonics;++i)
        SphericalHarmonics[i] = 0.0;    
    //    Order = (int)(-3/2+std::sqrt((float)(9/4-2*(1-NumberOfSphericalHarmonics)))); //    Wrong and redundant !!!
    
    this->UpdateSphericalHarmonicSource();
    this->Modified();
}

int*
vtkSphericalHarmonicSource::GetTesselationRange() {
    int* range = new int[2];
    GetTesselationRange(range);
    return range;
}

void
vtkSphericalHarmonicSource::GetTesselationRange(int *range) {
    range[0] = 2;
    range[1] = 5;
}

void
vtkSphericalHarmonicSource::SetSphericalHarmonicComponent(int i,double v) {
    if (SphericalHarmonics[i]!=v) {
        vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Spherical Harmonics coefficient "<< i <<" to "<< v);
        this->Modified(); 
        SphericalHarmonics[i] = v; 
    }
}

vtkCxxSetObjectMacro(vtkSphericalHarmonicSource,RotationMatrix,vtkMatrix4x4);

int
vtkSphericalHarmonicSource::RequestData(vtkInformation *vtkNotUsed(request),vtkInformationVector **vtkNotUsed(inputVector),vtkInformationVector *outputVector) {

    // Get the info and output objects.

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPolyData*    output  = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    
    // SH Source stuff.

    vtkFloatArray* sValues = vtkFloatArray::New();
    sValues->SetNumberOfTuples(shell->GetNumberOfPoints());
    sValues->SetNumberOfComponents(1);

    /* Project data on the sphere:

    In matrix form:
    S := spherical function (discrete data on the sphere)
    C := Spherical harmonic coefficients
    B := Basis function 

    S = CB
    */
    int N = shell->GetNumberOfPoints();
    
    matrix<double> C(1,NumberOfSphericalHarmonics);
    for (int i=0;i<NumberOfSphericalHarmonics;++i)
        C(0,i) = SphericalHarmonics[i];
    matrix<double> S = prod(C,BasisFunction);

    if (Normalize) {

        typedef matrix<double>::array_type::const_iterator iterator;
        std::pair<iterator,iterator> result = boost::minmax_element(S.data().begin(),S.data().end());
        
        double min = *(result.first);
        double max = *(result.second);

        if (max!=min) {
            for (unsigned i=0;i<S.size1();++i)
                for (unsigned j=0;j<S.size2();++j)
                    S(i,j) -= min;
            S /= (max-min);
        } else {
            for (unsigned i=0;i<S.size1();++i)
                for (unsigned j=0;j<S.size2();++j)
                    S(i,j) = 1.0;
        }
    }
    S *= this->Radius;
    
    for(int i=0;i<N;++i) {
        if  (i%10000==0) {
            this->UpdateProgress ((vtkFloatingPointType)i/N);
            if (this->GetAbortExecute())
                break;
        }
        sValues->SetTuple1(i,S(0,i));
    }

    shell->GetPointData()->SetScalars(sValues);
    sValues->Delete();

    // Don't know how to copy everything but the points.

    output->DeepCopy(shell);
    output->GetPoints()->Reset();

    TranslateAndDeformShell(shell,output->GetPoints(),this->Center,Deform,RotationMatrix);

    /* This is not copying enough!!! */
    //  output->SetPoints(shell->GetPoints());
    //  output->GetPointData()->SetScalars(shell->GetPointData()->GetScalars());
    //  output->GetPointData()->SetNormals(shell->GetPointData()->GetNormals());
    //  output->SetPolys(shell->GetPolys());

    return 1;
}

void
vtkSphericalHarmonicSource::PrintSelf(ostream& os,vtkIndent indent) {
    this->Superclass::PrintSelf(os,indent);
    
    os << indent << "Tessellation Order: " << Tesselation << std::endl;
    os << indent << "Tessellation Type: " << TesselationType << std::endl;
    os << indent << "Tessellation Basis: " << TesselationBasis << std::endl;

    os << indent << "SH Basis Order: " << Order << std::endl;
    os << indent << "Length of SH coefficient vector: " << NumberOfSphericalHarmonics << std::endl;
    os << indent << "SH Basis: " << NumberOfSphericalHarmonics << "x" << shell->GetNumberOfPoints() << std::endl;
    os << indent << "SH Coefficients:" << std::endl << '[';
    for (int i=0;i<NumberOfSphericalHarmonics;++i)
        os << indent << SphericalHarmonics[i] << " ";
    os << indent << ']' << std::endl;
}

int
vtkSphericalHarmonicSource::RequestInformation(vtkInformation*,vtkInformationVector**,vtkInformationVector *outputVector) {

    // Get the info object

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
                 this->Center[0]-this->Radius,this->Center[0]+this->Radius,
                 this->Center[1]-this->Radius,this->Center[1]+this->Radius,
                 this->Center[2]-this->Radius,this->Center[2]+this->Radius);
    
    return 1;
}

direction
Cartesian2Spherical(const direction v) {

    const double r = v.norm();

    // theta computation.

    if (r==0) {
        std::cerr << "Cannot have a 0 radius in spherical coordinates!\n" << std::endl;
        exit(1);
    }

    const double theta = acos(v.z/r);  /*this value is between 0 and PI*/

    // phi computation.

    // atan2 is the same as atan(y/x) but looks at the sign of x and y to determine the quadrant of the answer.
    // It returns a value between -PI and PI.

    const double phi = atan2(v.y,v.x);  // This value is between 0 and PI.

    const direction s = { r, phi, theta};

    return s;
}

matrix<double>
ComputeSHMatrix(const int order,vtkPolyData* shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections) {
    const int n_s   = shell->GetNumberOfPoints();
    //const int order = static_cast<int>(-3/2+std::sqrt(static_cast<float>(9/4-2*(1-rank)))); //  This is probably wrong !!!
    const int rank =  (order+1)*(order+2)/2;

    matrix<double> B(rank,n_s);

    vtkPoints* vertices = shell->GetPoints();

    for (int i=0;i<n_s;++i) {
        // Get spherical component of the point direction and transform them in spherical coordinates.

        double p[3];
        vertices->GetPoint(i,p);
        
        direction d;
        
        d.x = (FlipX) ? -p[0] : p[0]; 
        d.y = (FlipY) ? -p[1] : p[1]; 
        d.z = (FlipZ) ? -p[2] : p[2];
        
        const direction v = Cartesian2Spherical(d);
        
        const double phi   = v.y;
        const double theta = v.z;
        double temp=0;
        PhiThetaDirections(i,0) = phi;
        PhiThetaDirections(i,1) = theta;

        for (int l=0,j=0;l<=order;l+=2) {

            //  Handle the case m=0
            B(j,i) = std::tr1::sph_legendre(l,0,theta);
            j = j+1;

            for(int m=1,s=-1;m<=l;++m,++j,s=-s) {
                temp = std::tr1::sph_legendre(l, m,theta)*std::sqrt(2.0);

                //-m Real
                B(j,i) = s*temp*(cos(m*phi));//like t3 at hardi.cpp but math simplified and with tr1

                //+m Imag
                B(++j,i) = temp*(sin(s*m*phi));//like t3 at hardi.cpp but math simplified and with tr1
            }
        }
    }
    
    return B;
}

matrix<double>
ComputeSHMatrixMaxThesis(const int order,vtkPolyData *shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections) {
    const int n_s   = shell->GetNumberOfPoints();
    //    const int order = static_cast<int>(-3/2+std::sqrt(static_cast<float>(9/4-2*(1-rank)))); //  This is wrong !!!
    const int rank =  (order+1)*(order+2)/2;

    matrix<double> B(rank,n_s);

    vtkPoints* vertices = shell->GetPoints();
    for (int i=0;i<n_s;++i) {

        // Get spherical component of the point direction and transform them in spherical coordinates.

        double p[3];
        vertices->GetPoint(i,p);
        
        direction d;
        
        d.x = (FlipX) ? -p[0] : p[0]; 
        d.y = (FlipY) ? -p[1] : p[1]; 
        d.z = (FlipZ) ? -p[2] : p[2];
        
        const direction v = Cartesian2Spherical(d);
        
        const double phi   = v.y;
        const double theta = v.z;

        PhiThetaDirections(i,0) = phi;
        PhiThetaDirections(i,1) = theta;
#if 0
        double testA=0, testB=0, test2=0;

        for (int l=0,j=0;l<=order;l+=2) {
            for(int m=-l;m<0;++m,++j){

                //                testA = std::tr1::sph_legendre(l,std::abs(m),theta)*cos(m*phi);
                //                testB = real(GetSH(l,-m,theta,phi));
                //                testA = std::tr1::sph_legendre(l,std::abs(m),theta)*sin(std::abs(m)*phi);
                //                testB = imag(GetSH(l,-m,theta,phi));

                testA = std::tr1::sph_legendre(l,std::abs(m),theta)*cos(m*phi)*std::sqrt(2.0);
                testB = std::sqrt(2.0)*real(GetSH(l,-m,theta,phi));

                if(std::abs(testA-testB)>=0.0000000000001)
                    std::cout << "error "<< testA-testB << std::endl;

                B(j,i)=testA;
            }

            testA = std::tr1::sph_legendre(l,0,theta);
            testB = real(GetSH(l,0,theta,phi));

            if((testA-testB)!=0)
                std::cout << "error "<< testA-testB << std::endl;

            B(j++,i)=testA;

            for(int m=1;m<=l;++m,++j){
                testA = std::tr1::sph_legendre(l, m,theta)*sin(m*phi)*std::sqrt(2.0);
                testB = std::sqrt(2.0)*imag(GetSH(l,m,theta,phi));

                if(std::abs(testA-testB)>=0.00000000000001)
                    std::cout << "error "<< testA-testB << std::endl;

                B(j,i)=testA;
            }
        }
#endif
        //  It is even nicer to compute the SH once (for m>0 and for m<0).
        //  The central term is given by the suite u_n
        //  TO
        double temp=0;
        for (int l=0,j=0;l<=order;l+=2,j+=2*l-1) {
            B(j,i) = std::tr1::sph_legendre(l,0,theta);

            for(int m=1,j1=j-1,j2=j+1;m<=l;++m,--j1,++j2) {
                temp = std::sqrt(2.0)*std::tr1::sph_legendre(l,m,theta);
                B(j1,i) = temp*cos(m*phi);

                B(j2,i) = temp*sin(m*phi);
            }
        }
    }

    return B;
}

matrix<double>
ComputeSHMatrixTournier(const int order,vtkPolyData *shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections) {
    const int n_s   = shell->GetNumberOfPoints();
    const int rank =  (order+1)*(order+2)/2;

    //    const int order = static_cast<int>(-3/2+std::sqrt(static_cast<float>(9/4-2*(1-rank)))); //  This is wrong !!!

    /*
      We declare the Bmatrix of sixe n_s x n_b.
    */
    matrix<double> B(rank,n_s);

    vtkPoints* vertices = shell->GetPoints();

    std::complex<float>  cplx_1;

    for(int i = 0; i < n_s; i++) {

        // Get spherical component of the point direction and transform them in spherical coordinates.

        double p[3];
        vertices->GetPoint(i,p);

        direction d;

        d.x = (FlipX) ? -p[0] : p[0];
        d.y = (FlipY) ? -p[1] : p[1];
        d.z = (FlipZ) ? -p[2] : p[2];

        const direction v = Cartesian2Spherical(d);

        const double phi   = v.y;
        const double theta = v.z;

        PhiThetaDirections(i,0) = phi;
        PhiThetaDirections(i,1) = theta;
        int j = 0;
        //counter for the j dimension of B
        //get spherical component of the direction vector
        double temp=0;

        for(int l = 0; l <= order; l+=2)
            for(int m = -l,s=1; m <= l; m++,s=-s) {
                temp = std::tr1::sph_legendre(l,std::abs(m),theta);
                if(m >= 0) { /* positive "m" SH */
                    B(j,i) =  temp*cos(m*phi);;
                }
                else { /* negative "m" SH  */
                    B(j,i) = s*temp*sin(m*phi);
                }

                j++;
            }
    }

    return B;
}

matrix<double>
ComputeSHMatrixRshBasis(const int order,vtkPolyData* shell,const bool FlipX,const bool FlipY,const bool FlipZ,matrix<double>& PhiThetaDirections) {
    const int n_s   = shell->GetNumberOfPoints();
    const int rank =  (order+1)*(order+2)/2;

    //    const int order = static_cast<int>(-3/2+std::sqrt(static_cast<float>(9/4-2*(1-rank)))); //  This is probably wrong !!!

    matrix<double> B(rank,n_s);

    vtkPoints* vertices = shell->GetPoints();

    for (int i=0;i<n_s;++i) {

        // Get spherical component of the point direction and transform them in spherical coordinates.

        double p[3];
        vertices->GetPoint(i,p);

        direction d;

        d.x = (FlipX) ? -p[0] : p[0];
        d.y = (FlipY) ? -p[1] : p[1];
        d.z = (FlipZ) ? -p[2] : p[2];

        const direction v = Cartesian2Spherical(d);

        const double phi   = v.y;
        const double theta = v.z;
        PhiThetaDirections(i,0) = phi;
        PhiThetaDirections(i,1) = theta;
        double temp=0;

        for (int l=0,j=0;l<=order;l+=2) {
            B(j,i)=std::tr1::sph_legendre(l,0,theta);
            j=j+1;

            for(int m=1,s=-1;m<=l;++m,++j,s=-s) {
                temp = std::tr1::sph_legendre(l, m,theta)*std::sqrt(2.0);
                //-m Real
                B(j,i)   = temp*(cos(m*phi));//like RshBasis.pdf Luke Bloy eq 1.2 but math simplified and with tr1
                //+m Imag
                B(++j,i) = temp*(sin(m*phi));
            }
        }
    }
    return B;
}

void
vtkSphericalHarmonicSource::SetSphericalHarmonics(double* _arg) {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Spherical Harmonics to " << _arg);
    this->SphericalHarmonics = _arg; 
    this->Modified(); 
}

void
vtkSphericalHarmonicSource::UpdateSphericalHarmonicSource() {

    if (shell)
        shell->Delete();
    shell = vtkPolyData::New(); 
    p_solid tesselationType ;

    switch (TesselationType) {
    case Cube:         { tesselationType = cube;         break; }
    case Dodecahedron: { tesselationType = dodecahedron; break; }
    case Icosahedron:  { tesselationType = icosahedron;  break; }
    case Octahedron:   { tesselationType = octahedron;   break; }
    case Tetrahedron:  { tesselationType = tetrahedron;  break; }
    }

    vtkSphereTesselator<double> sMesh(tesselationType);

    sMesh.tesselate(Tesselation);
    sMesh.getvtkTesselation(shell);

    matrix<double> PhiThetaDirection(shell->GetNumberOfPoints(),2);

    switch (TesselationBasis) {
    case SHMatrix:
    {
        BasisFunction = ComputeSHMatrix(Order,shell,FlipX,FlipY,FlipZ,PhiThetaDirection); break;

    }
    case SHMatrixMaxThesis:
    {
        BasisFunction = ComputeSHMatrixMaxThesis(Order,shell,FlipX,FlipY,FlipZ,PhiThetaDirection); break;

    }
    case SHMatrixTournier:
    {
        BasisFunction = ComputeSHMatrixTournier(Order,shell,FlipX,FlipY,FlipZ,PhiThetaDirection); break;

    }
    case SHMatrixRshBasis:
    {
        BasisFunction = ComputeSHMatrixRshBasis(Order,shell,FlipX,FlipY,FlipZ,PhiThetaDirection); break;
    }
    }
    PhiThetaShellDirections = PhiThetaDirection;
}

void
TranslateAndDeformShell(vtkPolyData *shell,vtkPoints* outPts,double center[3],bool deform,vtkMatrix4x4* transform) {  
    
    vtkPoints* inPts = shell->GetPoints();
    const int  n     = inPts->GetNumberOfPoints();

    double range[2];
    shell->GetPointData()->GetScalars()->GetRange(range);
    
    const double rangeDiff = (range[0]!=range[1]) ? range[1]-range[0] : 1.;
    vtkDataArray* sValues  = shell->GetPointData()->GetScalars();

    for (int i=0;i<n;++i) {
        double point[4];
        inPts->GetPoint(i,point);

        if (deform) {
            const double val = sValues->GetTuple1(i);
            point[0] = ((val-range[0])/rangeDiff)*point[0];
            point[1] = ((val-range[0])/rangeDiff)*point[1];
            point[2] = ((val-range[0])/rangeDiff)*point[2];
        } 
        point[3] = 1.0;




        const double* pointOut = (transform!=0) ? transform->MultiplyDoublePoint(point) : &point[0];
        outPts->InsertNextPoint(pointOut[0]+center[0],pointOut[1]+center[1],pointOut[2]+center[2]);
    }
}
