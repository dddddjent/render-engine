#pragma once

#include <vtkSmartPointer.h>
#include <vtkXMLImageDataReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

template<typename T>
std::vector<T> readVti(const std::string &filename, const std::string field_name,
    std::function<T (T)> process_elements = nullptr)
{
    if(process_elements == nullptr) {
        process_elements = [](T v) -> T { return v; };
    }
    
    auto reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
    reader->SetFileName(filename.c_str());
    reader->Update();
    
    vtkDataArray* dataArray;
    auto imageData = reader->GetOutput();
    auto pointData = imageData->GetPointData();
    dataArray = pointData->GetArray(field_name.c_str());
    
    const vtkIdType numTuples = dataArray->GetNumberOfTuples();

    std::vector<T> field(numTuples);
    for (vtkIdType i = 0; i < numTuples; ++i) {
        field[i] = process_elements(static_cast<T>(dataArray->GetTuple1(i)));
    }

    return field;
}