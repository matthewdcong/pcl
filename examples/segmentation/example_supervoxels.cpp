#include <thread>

#include <pcl/common/time.h>
#include <pcl/console/parse.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/png_io.h>
#include <pcl/io/pcd_io.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/segmentation/supervoxel_clustering.h>

#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>
#include <vtkImageData.h>
#include <vtkImageFlip.h>
#include <vtkPolyLine.h>

using namespace std::chrono_literals;

// Types
using PointT = pcl::PointXYZRGBA;
using PointCloudT = pcl::PointCloud<PointT>;
using PointNT = pcl::PointNormal;
using PointNCloudT = pcl::PointCloud<PointNT>;
using PointLT = pcl::PointXYZL;
using PointLCloudT = pcl::PointCloud<PointLT>;
using NormalT = pcl::Normal;
using NormalCloudT = pcl::PointCloud<NormalT>;

bool show_voxel_centroids = true;
bool show_supervoxels = true;
bool show_supervoxel_normals = false;
bool show_graph = false;
bool show_normals = false;
bool show_refined = false;
bool show_help = true;

/** \brief Callback for setting options in the visualizer via keyboard.
 *  \param[in] event Registered keyboard event  */
void 
keyboard_callback (const pcl::visualization::KeyboardEvent& event, void*)
{
  int key = event.getKeyCode ();
  
  if (event.keyUp ())    
    switch (key)
    {
      case (int)'1': show_voxel_centroids = !show_voxel_centroids; break;
      case (int)'2': show_supervoxels = !show_supervoxels; break;
      case (int)'3': show_graph = !show_graph; break;
      case (int)'4': show_normals = !show_normals; break;
      case (int)'5': show_supervoxel_normals = !show_supervoxel_normals; break;
      case (int)'0': show_refined = !show_refined; break;
      case (int)'h': case (int)'H': show_help = !show_help; break;
      default: break;
    }
    
}

void addSupervoxelConnectionsToViewer (PointT &supervoxel_center, 
                                       PointCloudT &adjacent_supervoxel_centers,
                                       std::string supervoxel_name,
                                       pcl::visualization::PCLVisualizer::Ptr & viewer);

/** \brief Displays info text in the specified PCLVisualizer
 *  \param[in] viewer_arg The PCLVisualizer to modify  */
void printText (pcl::visualization::PCLVisualizer::Ptr viewer);

/** \brief Removes info text in the specified PCLVisualizer
 *  \param[in] viewer_arg The PCLVisualizer to modify  */
void removeText (pcl::visualization::PCLVisualizer::Ptr viewer);

/** \brief Checks if the PCLPointCloud2 pc2 has the field named field_name
 * \param[in] pc2 PCLPointCloud2 to check
 * \param[in] field_name Fieldname to check
 * \return True if field has been found, false otherwise */
bool
hasField (const pcl::PCLPointCloud2 &pc2, const std::string &field_name);


using namespace pcl;

int
main (int argc, char ** argv)
{
  if (argc < 2)
  {
    pcl::console::print_info ("Syntax is: %s {-p <pcd-file> OR -r <rgb-file> -d <depth-file>} \n --NT  (disables use of single camera transform) \n -o <output-file> \n -O <refined-output-file> \n-l <output-label-file> \n -L <refined-output-label-file> \n-v <voxel resolution> \n-s <seed resolution> \n-c <color weight> \n-z <spatial weight> \n-n <normal_weight>] \n", argv[0]);
    return (1);
  }
  
  ///////////////////////////////  //////////////////////////////
  //////////////////////////////  //////////////////////////////
  ////// THIS IS ALL JUST INPUT HANDLING - Scroll down until 
  ////// pcl::SupervoxelClustering<pcl::PointXYZRGB> super
  //////////////////////////////  //////////////////////////////
  std::string rgb_path;
  bool rgb_file_specified = pcl::console::find_switch (argc, argv, "-r");
  if (rgb_file_specified)
    pcl::console::parse (argc, argv, "-r", rgb_path);
  
  std::string depth_path;
  bool depth_file_specified = pcl::console::find_switch (argc, argv, "-d");
  if (depth_file_specified)
    pcl::console::parse (argc, argv, "-d", depth_path);
  
  PointCloudT::Ptr cloud (new PointCloudT);
  NormalCloudT::Ptr input_normals (new NormalCloudT);
  
  bool pcd_file_specified = pcl::console::find_switch (argc, argv, "-p");
  std::string pcd_path;
  if (!depth_file_specified || !rgb_file_specified)
  {
    std::cout << "Using point cloud\n";
    if (!pcd_file_specified)
    {
      std::cout << "No cloud specified!\n";
      return (1);
    }
    pcl::console::parse (argc,argv,"-p",pcd_path);
  }
  
  bool disable_transform = pcl::console::find_switch (argc, argv, "--NT");
  bool ignore_provided_normals = pcl::console::find_switch (argc, argv, "--nonormals");
  bool has_normals = false;
  
  std::string out_path = "test_output.png";;
  pcl::console::parse (argc, argv, "-o", out_path);
  
  std::string out_label_path = "test_output_labels.png";
  pcl::console::parse (argc, argv, "-l", out_label_path);
  
  std::string refined_out_path = "refined_test_output.png";
  pcl::console::parse (argc, argv, "-O", refined_out_path);
  
  std::string refined_out_label_path = "refined_test_output_labels.png";;
  pcl::console::parse (argc, argv, "-L", refined_out_label_path);

  float voxel_resolution = 0.008f;
  pcl::console::parse (argc, argv, "-v", voxel_resolution);
    
  float seed_resolution = 0.08f;
  pcl::console::parse (argc, argv, "-s", seed_resolution);
  
  float color_importance = 0.2f;
  pcl::console::parse (argc, argv, "-c", color_importance);
  
  float spatial_importance = 0.4f;
  pcl::console::parse (argc, argv, "-z", spatial_importance);
  
  float normal_importance = 1.0f;
  pcl::console::parse (argc, argv, "-n", normal_importance);
  
  if (!pcd_file_specified)
  {
    //Read the images
    vtkSmartPointer<vtkImageReader2Factory> reader_factory = vtkSmartPointer<vtkImageReader2Factory>::New ();
    vtkImageReader2* rgb_reader = reader_factory->CreateImageReader2 (rgb_path.c_str ());
    //qDebug () << "RGB File="<< QString::fromStdString(rgb_path);
    if ( ! rgb_reader->CanReadFile (rgb_path.c_str ()))
    {
      std::cout << "Cannot read rgb image file!";
      return (1);
    }
    rgb_reader->SetFileName (rgb_path.c_str ());
    rgb_reader->Update ();
    //qDebug () << "Depth File="<<QString::fromStdString(depth_path);
    vtkImageReader2* depth_reader = reader_factory->CreateImageReader2 (depth_path.c_str ());
    if ( ! depth_reader->CanReadFile (depth_path.c_str ()))
    {
      std::cout << "Cannot read depth image file!";
      return (1);
    }
    depth_reader->SetFileName (depth_path.c_str ());
    depth_reader->Update ();
    
    vtkSmartPointer<vtkImageFlip> flipXFilter = vtkSmartPointer<vtkImageFlip>::New();
    flipXFilter->SetFilteredAxis(0); // flip x axis
    flipXFilter->SetInputConnection(rgb_reader->GetOutputPort());
    flipXFilter->Update();
    
    vtkSmartPointer<vtkImageFlip> flipXFilter2 = vtkSmartPointer<vtkImageFlip>::New();
    flipXFilter2->SetFilteredAxis(0); // flip x axis
    flipXFilter2->SetInputConnection(depth_reader->GetOutputPort());
    flipXFilter2->Update();
    
    vtkSmartPointer<vtkImageData> rgb_image = flipXFilter->GetOutput ();
    int *rgb_dims = rgb_image->GetDimensions ();
    vtkSmartPointer<vtkImageData> depth_image = flipXFilter2->GetOutput ();
    int *depth_dims = depth_image->GetDimensions ();
    
    if (rgb_dims[0] != depth_dims[0] || rgb_dims[1] != depth_dims[1])
    {
      std::cout << "Depth and RGB dimensions to not match!";
      std::cout << "RGB Image is of size "<<rgb_dims[0] << " by "<<rgb_dims[1];
      std::cout << "Depth Image is of size "<<depth_dims[0] << " by "<<depth_dims[1];
      return (1);
    }
 
    cloud->points.reserve (depth_dims[0] * depth_dims[1]);
    cloud->width = depth_dims[0];
    cloud->height = depth_dims[1];
    cloud->is_dense = false;
    
    
    // Fill in image data
    int centerX = static_cast<int>(cloud->width / 2.0);
    int centerY = static_cast<int>(cloud->height / 2.0);
    unsigned short* depth_pixel;
    unsigned char* color_pixel;
    float scale = 1.0f/1000.0f;
    float focal_length = 525.0f;
    float fl_const = 1.0f / focal_length;
    depth_pixel = static_cast<unsigned short*>(depth_image->GetScalarPointer (depth_dims[0]-1,depth_dims[1]-1,0));
    color_pixel = static_cast<unsigned char*> (rgb_image->GetScalarPointer (depth_dims[0]-1,depth_dims[1]-1,0));
    
    for (size_t y=0; y<cloud->height; ++y)
    {
      for (size_t x=0; x<cloud->width; ++x, --depth_pixel, color_pixel-=3)
      {
        PointT new_point;
        //  uint8_t* p_i = &(cloud_blob->data[y * cloud_blob->row_step + x * cloud_blob->point_step]);
        float depth = static_cast<float>(*depth_pixel) * scale;
        if (depth == 0.0f)
        {
          new_point.x = new_point.y = new_point.z = std::numeric_limits<float>::quiet_NaN ();
        }
        else
        {
          new_point.x = (static_cast<float> (x) - centerX) * depth * fl_const;
          new_point.y = (static_cast<float> (centerY) - y) * depth * fl_const; // vtk seems to start at the bottom left image corner
          new_point.z = depth;
        }
        
        new_point.r = color_pixel[0];
        new_point.g = color_pixel[1];
        new_point.b = color_pixel[2];
        cloud->points.push_back (new_point);
        
      }
    }
  }
  else
  {
    /// check if the provided pcd file contains normals
    pcl::PCLPointCloud2 input_pointcloud2;
    if (pcl::io::loadPCDFile (pcd_path, input_pointcloud2))
    {
      PCL_ERROR ("ERROR: Could not read input point cloud %s.\n", pcd_path.c_str ());
      return (3);
    }
    pcl::fromPCLPointCloud2 (input_pointcloud2, *cloud);
    if (!ignore_provided_normals)
    {
      if (hasField (input_pointcloud2,"normal_x"))
      {
        std::cout << "Using normals contained in file. Set --nonormals option to disable this.\n";
        pcl::fromPCLPointCloud2 (input_pointcloud2, *input_normals);
        has_normals = true;
      }
    }
  }
  std::cout << "Done making cloud!\n";

  ///////////////////////////////  //////////////////////////////
  //////////////////////////////  //////////////////////////////
  ////// This is how to use supervoxels
  //////////////////////////////  //////////////////////////////
  //////////////////////////////  //////////////////////////////
  
  // If the cloud is organized and we haven't disabled the transform we need to
  // check that there are no negative z values, since we use std::log(z)
  if (cloud->isOrganized () && !disable_transform)
  {
    for (const auto &point : *cloud)
      if (point.z < 0)
      {
        PCL_ERROR ("Points found with negative Z values, this is not compatible with the single camera transform!\n");
        PCL_ERROR ("Set the --NT option to disable the single camera transform!\n");
        return 1;
      }
    std::cout <<"You have the single camera transform enabled - this should be used with point clouds captured from a single camera.\n";
    std::cout <<"You can disable the transform with the --NT flag\n";    
  }
  
  pcl::SupervoxelClustering<PointT> super (voxel_resolution, seed_resolution);
  //If we manually disabled the transform then do so, otherwise the default 
  //behavior will take place (true for organized, false for unorganized)
  if (disable_transform)
    super.setUseSingleCameraTransform (false);
  super.setInputCloud (cloud);
  if (has_normals)
    super.setNormalCloud (input_normals);
  super.setColorImportance (color_importance);
  super.setSpatialImportance (spatial_importance);
  super.setNormalImportance (normal_importance);
  std::map <uint32_t, pcl::Supervoxel<PointT>::Ptr > supervoxel_clusters;
 
  std::cout << "Extracting supervoxels!\n";
  super.extract (supervoxel_clusters);
  std::cout << "Found " << supervoxel_clusters.size () << " Supervoxels!\n";
  PointLCloudT::Ptr labeled_voxel_cloud = super.getLabeledVoxelCloud ();
  PointCloudT::Ptr voxel_centroid_cloud = super.getVoxelCentroidCloud ();
  PointNCloudT::Ptr sv_normal_cloud = pcl::SupervoxelClustering<PointT>::makeSupervoxelNormalCloud (supervoxel_clusters);
  PointLCloudT::Ptr full_labeled_cloud = super.getLabeledCloud ();
  
  std::cout << "Getting supervoxel adjacency\n";
  std::multimap<uint32_t, uint32_t> label_adjacency;
  super.getSupervoxelAdjacency (label_adjacency);
   
  std::map <uint32_t, pcl::Supervoxel<PointT>::Ptr > refined_supervoxel_clusters;
  std::cout << "Refining supervoxels \n";
  super.refineSupervoxels (3, refined_supervoxel_clusters);

  PointLCloudT::Ptr refined_labeled_voxel_cloud = super.getLabeledVoxelCloud ();
  PointNCloudT::Ptr refined_sv_normal_cloud = pcl::SupervoxelClustering<PointT>::makeSupervoxelNormalCloud (refined_supervoxel_clusters);
  PointLCloudT::Ptr refined_full_labeled_cloud = super.getLabeledCloud ();
  
  // THESE ONLY MAKE SENSE FOR ORGANIZED CLOUDS
  if (cloud->isOrganized ())
  {
    pcl::io::savePNGFile (out_label_path, *full_labeled_cloud, "label");
    pcl::io::savePNGFile (refined_out_label_path, *refined_full_labeled_cloud, "label");
    //Save RGB from labels
    pcl::io::PointCloudImageExtractorFromLabelField<PointLT> pcie (pcl::io::PointCloudImageExtractorFromLabelField<PointLT>::COLORS_RGB_GLASBEY);
    //We need to set this to account for NAN points in the organized cloud
    pcie.setPaintNaNsWithBlack (true);
    pcl::PCLImage image;
    pcie.extract (*full_labeled_cloud, image);
    pcl::io::savePNGFile (out_path, image);
    pcie.extract (*refined_full_labeled_cloud, image);
    pcl::io::savePNGFile (refined_out_path, image);
  }
  
  std::cout << "Constructing Boost Graph Library Adjacency List...\n";
  using VoxelAdjacencyList = boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, uint32_t, float>;
  VoxelAdjacencyList supervoxel_adjacency_list;
  super.getSupervoxelAdjacencyList (supervoxel_adjacency_list);

  
  std::cout << "Loading visualization...\n";
  pcl::visualization::PCLVisualizer::Ptr viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
  viewer->setBackgroundColor (0, 0, 0);
  viewer->registerKeyboardCallback(keyboard_callback, nullptr);

 
  bool refined_normal_shown = show_refined;
  bool refined_sv_normal_shown = show_refined;
  bool sv_added = false;
  bool normals_added = false;
  bool graph_added = false;
  std::vector<std::string> poly_names;
  std::cout << "Loading viewer...\n";
  while (!viewer->wasStopped ())
  {
    if (show_supervoxels)
    {
      if (!viewer->updatePointCloud ((show_refined)?refined_labeled_voxel_cloud:labeled_voxel_cloud, "colored voxels"))
      {
        viewer->addPointCloud ((show_refined)?refined_labeled_voxel_cloud:labeled_voxel_cloud, "colored voxels");
        viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE,3.0, "colored voxels");
      }
    }
    else
    {
      viewer->removePointCloud ("colored voxels");
    }
    
    if (show_voxel_centroids)
    {
      if (!viewer->updatePointCloud (voxel_centroid_cloud, "voxel centroids"))
      {
        viewer->addPointCloud (voxel_centroid_cloud, "voxel centroids");
        viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE,2.0, "voxel centroids");
      }
    }
    else
    {
      viewer->removePointCloud ("voxel centroids");
    }

    if (show_supervoxel_normals)
    {
      if (refined_sv_normal_shown != show_refined || !sv_added)
      {
        viewer->removePointCloud ("supervoxel_normals");
        viewer->addPointCloudNormals<PointNormal> ((show_refined)?refined_sv_normal_cloud:sv_normal_cloud,1,0.05f, "supervoxel_normals");
        sv_added = true;
      }
      refined_sv_normal_shown = show_refined;
    }
    else if (!show_supervoxel_normals)
    {
      viewer->removePointCloud ("supervoxel_normals");
    }
    
    if (show_normals)
    {
      auto sv_itr = ((show_refined) ? refined_supervoxel_clusters.begin () : supervoxel_clusters.begin ());
      auto sv_itr_end = ((show_refined) ? refined_supervoxel_clusters.end () : supervoxel_clusters.end ());
      for (; sv_itr != sv_itr_end; ++sv_itr)
      {
        std::stringstream ss;
        ss << sv_itr->first <<"_normal";
        if (refined_normal_shown != show_refined || !normals_added)
        {
          viewer->removePointCloud (ss.str ());
          viewer->addPointCloudNormals<PointT,Normal> ((sv_itr->second)->voxels_,(sv_itr->second)->normals_,10,0.02f,ss.str ());
        //  std::cout << (sv_itr->second)->normals_->points[0]<<"\n";
          
        }
          
      }
      normals_added = true;
      refined_normal_shown = show_refined;
    }
    else if (!show_normals)
    {
      auto sv_itr = ((show_refined) ? refined_supervoxel_clusters.begin () : supervoxel_clusters.begin ());
      auto sv_itr_end = ((show_refined) ? refined_supervoxel_clusters.end () : supervoxel_clusters.end ());
      for (; sv_itr != sv_itr_end; ++sv_itr)
      {
        std::stringstream ss;
        ss << sv_itr->first << "_normal";
        viewer->removePointCloud (ss.str ());
      }
    }
    
    if (show_graph && !graph_added)
    {
      poly_names.clear ();
      for (auto label_itr = label_adjacency.begin (); label_itr != label_adjacency.end (); )
      {
        //First get the label 
        uint32_t supervoxel_label = label_itr->first;
         //Now get the supervoxel corresponding to the label
        pcl::Supervoxel<PointT>::Ptr supervoxel = supervoxel_clusters.at (supervoxel_label);
        //Now we need to iterate through the adjacent supervoxels and make a point cloud of them
        PointCloudT adjacent_supervoxel_centers;
        for (auto adjacent_itr = label_adjacency.equal_range (supervoxel_label).first; adjacent_itr!=label_adjacency.equal_range (supervoxel_label).second; ++adjacent_itr)
        {     
          pcl::Supervoxel<PointT>::Ptr neighbor_supervoxel = supervoxel_clusters.at (adjacent_itr->second);
          adjacent_supervoxel_centers.push_back (neighbor_supervoxel->centroid_);
        }
        //Now we make a name for this polygon
        std::stringstream ss;
        ss << "supervoxel_" << supervoxel_label;
        poly_names.push_back (ss.str ());
        addSupervoxelConnectionsToViewer (supervoxel->centroid_, adjacent_supervoxel_centers, ss.str (), viewer);  
        //Move iterator forward to next label
        label_itr = label_adjacency.upper_bound (supervoxel_label);
      }
        
      graph_added = true;
    }
    else if (!show_graph && graph_added)
    {
      for (const auto &poly_name : poly_names)
      {
        viewer->removeShape (poly_name);
      }
      graph_added = false;
    }
    
    if (show_help)
    {
      viewer->removeShape ("help_text");
      printText (viewer);
    }
    else
    {
      removeText (viewer);
      if (!viewer->updateText("Press h to show help", 5, 10, 12, 1.0, 1.0, 1.0,"help_text") )
        viewer->addText("Press h to show help", 5, 10, 12, 1.0, 1.0, 1.0,"help_text");
    }
      
    
    viewer->spinOnce (100);
    std::this_thread::sleep_for(100ms);
    
  }
  return (0);
}

void
addSupervoxelConnectionsToViewer (PointT &supervoxel_center, 
                                  PointCloudT &adjacent_supervoxel_centers,
                                  std::string supervoxel_name,
                                  pcl::visualization::PCLVisualizer::Ptr & viewer)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New (); 
  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New (); 
  vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New ();
  
  //Iterate through all adjacent points, and add a center point to adjacent point pair
  for (const auto &adjacent_supervoxel_center : adjacent_supervoxel_centers)
  {
    points->InsertNextPoint (supervoxel_center.data);
    points->InsertNextPoint (adjacent_supervoxel_center.data); 
  } 
  // Create a polydata to store everything in
  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New ();
  // Add the points to the dataset
  polyData->SetPoints (points);
  polyLine->GetPointIds  ()->SetNumberOfIds(points->GetNumberOfPoints ());
  for(vtkIdType i = 0; i < points->GetNumberOfPoints (); i++)
    polyLine->GetPointIds ()->SetId (i,i);
  cells->InsertNextCell (polyLine);
  // Add the lines to the dataset
  polyData->SetLines (cells);
  viewer->addModelFromPolyData (polyData,supervoxel_name);
}


void printText (pcl::visualization::PCLVisualizer::Ptr viewer)
{
  std::string on_str = "on";
  std::string off_str = "off";
  if (!viewer->updateText ("Press (1-n) to show different elements (h) to disable this", 5, 72, 12, 1.0, 1.0, 1.0,"hud_text"))
    viewer->addText ("Press 1-n to show different elements", 5, 72, 12, 1.0, 1.0, 1.0,"hud_text");
  
  std::string temp = "(1) Voxels currently " + ((show_voxel_centroids)?on_str:off_str);
  if (!viewer->updateText (temp, 5, 60, 10, 1.0, 1.0, 1.0, "voxel_text"))
    viewer->addText (temp, 5, 60, 10, 1.0, 1.0, 1.0, "voxel_text");
  
  temp = "(2) Supervoxels currently "+ ((show_supervoxels)?on_str:off_str);
  if (!viewer->updateText (temp, 5, 50, 10, 1.0, 1.0, 1.0, "supervoxel_text") )
    viewer->addText (temp, 5, 50, 10, 1.0, 1.0, 1.0, "supervoxel_text");
  
  temp = "(3) Graph currently "+ ((show_graph)?on_str:off_str);
  if (!viewer->updateText (temp, 5, 40, 10, 1.0, 1.0, 1.0, "graph_text") )
    viewer->addText (temp, 5, 40, 10, 1.0, 1.0, 1.0, "graph_text");
  
  temp = "(4) Voxel Normals currently "+ ((show_normals)?on_str:off_str);
  if (!viewer->updateText (temp, 5, 30, 10, 1.0, 1.0, 1.0, "voxel_normals_text") )
    viewer->addText (temp, 5, 30, 10, 1.0, 1.0, 1.0, "voxel_normals_text");
  
  temp = "(5) Supervoxel Normals currently "+ ((show_supervoxel_normals)?on_str:off_str);
  if (!viewer->updateText (temp, 5, 20, 10, 1.0, 1.0, 1.0, "supervoxel_normals_text") )
    viewer->addText (temp, 5, 20, 10, 1.0, 1.0, 1.0, "supervoxel_normals_text");
  
  temp = "(0) Showing "+ std::string((show_refined)?"":"UN-") + "refined supervoxels and normals";
  if (!viewer->updateText (temp, 5, 10, 10, 1.0, 1.0, 1.0, "refined_text") )
    viewer->addText (temp, 5, 10, 10, 1.0, 1.0, 1.0, "refined_text");
}

void removeText (pcl::visualization::PCLVisualizer::Ptr viewer)
{
  viewer->removeShape ("hud_text");
  viewer->removeShape ("voxel_text");
  viewer->removeShape ("supervoxel_text");
  viewer->removeShape ("graph_text");
  viewer->removeShape ("voxel_normals_text");
  viewer->removeShape ("supervoxel_normals_text");
  viewer->removeShape ("refined_text");
}

bool
hasField (const pcl::PCLPointCloud2 &pc2, const std::string &field_name)
{
  for (const auto &field : pc2.fields)
    if (field.name == field_name)
      return true;
  return false;
}
