#pragma once

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLImageDataReader.h>

template <typename T>
std::vector<T> readVti(const std::string& filename, const std::string field_name,
                       std::function<T(T)> process_elements = nullptr)
{
    if (process_elements == nullptr) {
        process_elements = [](T v) -> T { return v; };
    }

    auto reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
    reader->SetFileName(filename.c_str());
    reader->Update();

    vtkDataArray* dataArray;
    auto imageData = reader->GetOutput();
    auto pointData = imageData->GetPointData();
    dataArray      = pointData->GetArray(field_name.c_str());

    const vtkIdType numTuples = dataArray->GetNumberOfTuples();

    std::vector<T> field(numTuples);
    for (vtkIdType i = 0; i < numTuples; ++i) {
        field[i] = process_elements(static_cast<T>(dataArray->GetTuple1(i)));
    }

    return field;
}

template <typename T>
std::vector<T> readVti(const std::string& filename, const std::string& field_name,
                       int component_index, 
                       std::function<T(T)> process_elements = nullptr)
{
    if (process_elements == nullptr) {
        process_elements = [](T v) -> T { return v; };
    }

    auto reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
    reader->SetFileName(filename.c_str());
    reader->Update();

    vtkDataArray* dataArray;
    auto imageData = reader->GetOutput();
    auto pointData = imageData->GetPointData();
    dataArray = pointData->GetArray(field_name.c_str());

    if (!dataArray) {
        throw std::runtime_error("Field '" + field_name + "' not found in the dataset.");
    }

    const int numComponents = dataArray->GetNumberOfComponents();
    if (component_index < 0 || component_index >= numComponents) {
        throw std::out_of_range("Invalid component index: " + std::to_string(component_index) +
                                ". Available components: 0 to " + std::to_string(numComponents - 1));
    }

    const vtkIdType numTuples = dataArray->GetNumberOfTuples();
    std::vector<T> field(numTuples);

    for (vtkIdType i = 0; i < numTuples; ++i) {
        field[i] = process_elements(static_cast<T>(dataArray->GetComponent(i, component_index)));
    }

    return field;
}
