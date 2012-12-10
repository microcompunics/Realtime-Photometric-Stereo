#include "camerawidget.h"

CameraWidget::CameraWidget(QWidget *parent, int width, int height) : QVTKWidget(parent) {

    renderWindow = GetRenderWindow();

    SetRenderWindow(renderWindow);
    renderWindow->SetNumberOfLayers(1);
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);
    renderer->SetLayer(0);
    renderer->InteractiveOff();

    cv::Mat tmpFrame(height, width, CV_8UC1, cv::Scalar::all(0));
    setup(tmpFrame);
    renderWindow->Render();
}

CameraWidget::~CameraWidget() {

}

void CameraWidget::setup(cv::Mat& image) {

    /* convert cv::Mat image to vtkImageData */
    imgData = vtkSmartPointer<vtkImageData>::New();
    importer = vtkSmartPointer<vtkImageImport>::New();

    importer->SetOutput(imgData);
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(0, 0, 0);
    importer->SetWholeExtent(0, image.cols-1, 0, image.rows-1, 0, 0);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToUnsignedChar();
    importer->SetNumberOfScalarComponents(image.channels());
    importer->SetImportVoidPointer(image.data);
    importer->Update();

    imgActor = vtkSmartPointer<vtkImageActor>::New();
    imgActor->SetInput(imgData);

    /* set up the orthogonal camera for the video layer */
    vtkSmartPointer<vtkCamera> orthoCam = renderer->GetActiveCamera();
    orthoCam->ParallelProjectionOn();

    double origin[3];
    double spacing[3];
    int extent[6];
    imgData->GetOrigin( origin );
    imgData->GetSpacing( spacing );
    imgData->GetExtent( extent );

    double xc = origin[0] + 0.5*(extent[0] + extent[1])*spacing[0];
    double yc = origin[1] + 0.5*(extent[2] + extent[3])*spacing[1];
    double yd = (extent[3] - extent[2] + 1)*spacing[1];
    double d = orthoCam->GetDistance();
    orthoCam->SetParallelScale(0.5*yd);

    /* flip the image vertically according to the vtk coordinate system */
    vtkSmartPointer<vtkMatrix4x4> flipMat = vtkSmartPointer<vtkMatrix4x4>::New();
    const double flipArray[] = {1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    flipMat->DeepCopy(flipArray);
    vtkSmartPointer<vtkMatrixToHomogeneousTransform> videoFlip = vtkSmartPointer<vtkMatrixToHomogeneousTransform>::New();
    videoFlip->SetInput(flipMat);

    orthoCam->SetUserTransform(videoFlip);
    orthoCam->SetFocalPoint(xc,yc,0.0);
    orthoCam->SetPosition(xc,yc,d);

    renderer->AddActor(imgActor);

}

void CameraWidget::setImage(cv::Mat image) {

    importer->SetImportVoidPointer(image.data);
    importer->Modified();
    update();
}
