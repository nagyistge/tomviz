def transform_scalars(dataset):
    """Erode segmented objects with a given label by a spherically symmetric
    structuring element with a given radius.
    """

    try:
        import itk
        from tomviz import utils
    except Exception as exc:
        print("Could not import necessary module(s)")
        print(exc)

    #----USER SPECIFIED VARIABLES-----#
    ###structuring_element_id### # (0 - Box, 1 - Ball, 2 - Cross)
    ###radius### # Specify the radius of the cube structuring element
    ###object_label### # Specify the label designating the segmented objects
    ###background_label### # Specify the label designating the background

    # Add a try/except around the ITK portion. ITK exceptions are
    # passed up to the Python layer, so we can at least report what
    # went wrong with the script, e.g,, unsupported image type.
    try:
        # Get the ITK image
        itk_image = utils.convert_vtk_to_itk_image(dataset)
        itk_input_image_type = type(itk_image)

        itk_kernel_type = itk.FlatStructuringElement[3]
        if (structuring_element_id == 0):
            itk_kernel = itk_kernel_type.Box(radius)
        elif (structuring_element_id == 1):
            itk_kernel = itk_kernel_type.Ball(radius)
        elif (structuring_element_id == 2):
            itk_kernel = itk_kernel_type.Cross(radius)
        else:
            raise Exception('Invalid kernel shape id %d' %
                            structuring_element_id)

        erode_filter = itk.BinaryErodeImageFilter[itk_input_image_type,
                                                  itk_input_image_type,
                                                  itk_kernel_type].New()
        erode_filter.SetErodeValue(object_label)
        erode_filter.SetBackgroundValue(background_label)
        erode_filter.SetKernel(itk_kernel)
        erode_filter.SetInput(itk_image)
        erode_filter.Update()
        itk_image_data = erode_filter.GetOutput()

        PyBufferType = itk.PyBuffer[itk_input_image_type]
        label_buffer = PyBufferType.GetArrayFromImage(itk_image_data)

        utils.set_array(dataset, label_buffer)
    except Exception as exc:
        print("Exception encountered while running BinaryErode")
        print(exc)
