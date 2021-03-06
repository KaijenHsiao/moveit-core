/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Mrinal Kalakrishnan, Ken Anderson, E. Gil Jones */

#include <moveit/distance_field/distance_field.h>

namespace distance_field
{

DistanceField::DistanceField(double resolution) :
  resolution_(resolution),
  inv_twice_resolution_(1.0/(2.0*resolution_))
{
}

DistanceField::~DistanceField()
{
}

double DistanceField::getDistanceGradient(double x, double y, double z, double& gradient_x, double& gradient_y, double& gradient_z,
                                          bool& in_bounds) const
{
  int gx, gy, gz;

  worldToGrid(x, y, z, gx, gy, gz);

  // if out of bounds, return 0 distance, and 0 gradient
  // we need extra padding of 1 to get gradients
  if (gx<1 || gy<1 || gz<1 || gx>=getXNumCells()-1 || gy>=getYNumCells()-1 || gz>=getZNumCells()-1)
  {
    gradient_x = 0.0;
    gradient_y = 0.0;
    gradient_z = 0.0;
    in_bounds = false;
    return 0;
  }

  gradient_x = (getDistanceFromCell(gx+1,gy,gz) - getDistanceFromCell(gx-1,gy,gz))*inv_twice_resolution_;
  gradient_y = (getDistanceFromCell(gx,gy+1,gz) - getDistanceFromCell(gx,gy-1,gz))*inv_twice_resolution_;
  gradient_z = (getDistanceFromCell(gx,gy,gz+1) - getDistanceFromCell(gx,gy,gz-1))*inv_twice_resolution_;

  in_bounds = true;
  return getDistanceFromCell(gx,gy,gz);
}

void DistanceField::getIsoSurfaceMarkers(double min_radius, double max_radius,
                                         const std::string & frame_id, const ros::Time stamp,
                                         const Eigen::Affine3d& cur,
                                         visualization_msgs::Marker& inf_marker ) const
{
  inf_marker.points.clear();
  inf_marker.header.frame_id = frame_id;
  inf_marker.header.stamp = stamp;
  inf_marker.ns = "distance_field";
  inf_marker.id = 1;
  inf_marker.type = visualization_msgs::Marker::CUBE_LIST;
  inf_marker.action = visualization_msgs::Marker::MODIFY;
  inf_marker.scale.x = resolution_;
  inf_marker.scale.y = resolution_;
  inf_marker.scale.z = resolution_;
  inf_marker.color.r = 1.0;
  inf_marker.color.g = 0.0;
  inf_marker.color.b = 0.0;
  inf_marker.color.a = 0.1;
  //inf_marker.lifetime = ros::Duration(30.0);

  inf_marker.points.reserve(100000);
  for (int x = 0; x < getXNumCells(); ++x)
  {
    for (int y = 0; y < getYNumCells(); ++y)
    {
      for (int z = 0; z < getZNumCells(); ++z)
      {
        double dist = getDistanceFromCell(x,y,z);
        
        if (dist >= min_radius && dist <= max_radius)
        {
          int last = inf_marker.points.size();
          inf_marker.points.resize(last + 1);
          double nx, ny, nz;
          gridToWorld(x,y,z,
                      nx, ny, nz);
          Eigen::Translation3d vec(nx,ny,nz);
          //Eigen::Translation3d nv = cur.rotation()*vec;
          inf_marker.points[last].x = vec.x();
          inf_marker.points[last].y = vec.y();
          inf_marker.points[last].z = vec.z();
        }
      }
    }
  }
}

void DistanceField::getGradientMarkers( double min_radius, double max_radius,
                                        const std::string & frame_id, const ros::Time stamp,
                                        std::vector<visualization_msgs::Marker>& markers ) const
{
  Eigen::Vector3d unitX(1, 0, 0);
  Eigen::Vector3d unitY(0, 1, 0);
  Eigen::Vector3d unitZ(0, 0, 1);

  int id = 0;

  for (int x = 0; x < getXNumCells(); ++x)
  {
    for (int y = 0; y < getYNumCells(); ++y)
    {
      for (int z = 0; z < getZNumCells(); ++z)
      {
        double worldX, worldY, worldZ;
        gridToWorld(x, y, z, worldX, worldY, worldZ);

        double gradientX, gradientY, gradientZ;
        bool in_bounds;
        double distance = getDistanceGradient(worldX, worldY, worldZ, gradientX, gradientY, gradientZ, in_bounds);
        Eigen::Vector3d gradient(gradientX, gradientY, gradientZ);

        if (in_bounds && distance >= min_radius && distance <= max_radius && gradient.norm() > 0)
        {
          visualization_msgs::Marker marker;

          marker.header.frame_id = frame_id;
          marker.header.stamp = stamp;

          marker.ns = "distance_field_gradient";
          marker.id = id++;
          marker.type = visualization_msgs::Marker::ARROW;
          marker.action = visualization_msgs::Marker::ADD;

          marker.pose.position.x = worldX;
          marker.pose.position.y = worldY;
          marker.pose.position.z = worldZ;

          Eigen::Vector3d axis = gradient.cross(unitX).norm() > 0 ? gradient.cross(unitX) : unitY;
          //double angle = -gradient.angle(unitX);
          //Eigen::AngleAxisd rotation(angle, axis);

          // marker.pose.orientation.x = rotation.rotation().x();
          // marker.pose.orientation.y = rotation.rotation().y();
          // marker.pose.orientation.z = rotation.rotation().z();
          // marker.pose.orientation.w = rotation.rotation().w();

          marker.scale.x = getResolution();
          marker.scale.y = getResolution();
          marker.scale.z = getResolution();

          marker.color.r = 0.0;
          marker.color.g = 0.0;
          marker.color.b = 1.0;
          marker.color.a = 1.0;

          markers.push_back(marker);
        }
      }
    }
  }
}

void DistanceField::addCollisionMapToField(const moveit_msgs::CollisionMap &collision_map)
{
  size_t num_boxes = collision_map.boxes.size();
  EigenSTL::vector_Vector3d points;
  points.reserve(num_boxes);
  for (size_t i=0; i<num_boxes; ++i)
  {
    points.push_back(Eigen::Vector3d(collision_map.boxes[i].pose.position.x,
                                     collision_map.boxes[i].pose.position.y,
                                     collision_map.boxes[i].pose.position.z
                                     ));
  }
  addPointsToField(points);
}

void DistanceField::getPlaneMarkers(distance_field::PlaneVisualizationType type, double length, double width,
                                    double height, const Eigen::Vector3d& origin,
                                    const std::string & frame_id, const ros::Time stamp,
                                    visualization_msgs::Marker& plane_marker ) const
{
  plane_marker.header.frame_id = frame_id;
  plane_marker.header.stamp = stamp;
  plane_marker.ns = "distance_field_plane";
  plane_marker.id = 1;
  plane_marker.type = visualization_msgs::Marker::CUBE_LIST;
  plane_marker.action = visualization_msgs::Marker::ADD;
  plane_marker.scale.x = resolution_;
  plane_marker.scale.y = resolution_;
  plane_marker.scale.z = resolution_;
  //plane_marker.lifetime = ros::Duration(30.0);

  plane_marker.points.reserve(100000);
  plane_marker.colors.reserve(100000);

  double minX = 0;
  double maxX = 0;
  double minY = 0;
  double maxY = 0;
  double minZ = 0;
  double maxZ = 0;

  switch(type)
  {
  case XYPlane:
    minZ = height;
    maxZ = height;

    minX = -length/2.0;
    maxX = length/2.0;
    minY = -width/2.0;
    maxY = width/2.0;
    break;
  case XZPlane:
    minY = height;
    maxY = height;

    minX = -length/2.0;
    maxX = length/2.0;
    minZ = -width/2.0;
    maxZ = width/2.0;
    break;
  case YZPlane:
    minX = height;
    maxX = height;

    minY = -length/2.0;
    maxY = length/2.0;
    minZ = -width/2.0;
    maxZ = width/2.0;
    break;
  }

  minX += origin.x();
  minY += origin.y();
  minZ += origin.z();

  maxX += origin.x();
  maxY += origin.y();
  maxZ += origin.z();

  int minXCell = 0;
  int maxXCell = 0;
  int minYCell = 0;
  int maxYCell = 0;
  int minZCell = 0;
  int maxZCell = 0;

  worldToGrid(minX,minY,minZ, minXCell, minYCell, minZCell);
  worldToGrid(maxX,maxY,maxZ, maxXCell, maxYCell, maxZCell);
  plane_marker.color.a = 1.0;
  for(int x = minXCell; x <= maxXCell; ++x)
  {
    for(int y = minYCell; y <= maxYCell; ++y)
    {
      for(int z = minZCell; z <= maxZCell; ++z)
      {
        if(!isCellValid(x,y,z))
        {
          continue;
        }
        double dist = getDistanceFromCell(x, y, z);
        int last = plane_marker.points.size();
        plane_marker.points.resize(last + 1);
        plane_marker.colors.resize(last + 1);
        double nx, ny, nz;
        gridToWorld(x, y, z, nx, ny, nz);
        Eigen::Vector3d vec(nx, ny, nz);
        plane_marker.points[last].x = vec.x();
        plane_marker.points[last].y = vec.y();
        plane_marker.points[last].z = vec.z();
        if(dist < 0.0)
        {
          plane_marker.colors[last].r = fmax(fmin(0.1/fabs(dist), 1.0), 0.0);
          plane_marker.colors[last].g = fmax(fmin(0.05/fabs(dist), 1.0), 0.0);
          plane_marker.colors[last].b = fmax(fmin(0.01/fabs(dist), 1.0), 0.0);

        }
        else
        {
          plane_marker.colors[last].b = fmax(fmin(0.1/(dist+0.001), 1.0),0.0);
          plane_marker.colors[last].g = fmax(fmin(0.05/(dist+0.001), 1.0),0.0);
          plane_marker.colors[last].r = fmax(fmin(0.01/(dist+0.001), 1.0),0.0);
        }
      }
    }
  }
}



void DistanceField::setPoint(const int xCell, const int yCell, const int zCell,
                             const double dist, geometry_msgs::Point & point,
                             std_msgs::ColorRGBA & color, const double max_distance) const 
{
  double wx,wy,wz;
  gridToWorld(xCell,yCell,zCell, wx,wy,wz);

  point.x = wx;
  point.y = wy;
  point.z = wz;

  color.r = 1.0;
  color.g = dist/max_distance;//dist/max_distance * 0.5;
  color.b = dist/max_distance;//dist/max_distance * 0.1;
}


void DistanceField::getProjectionPlanes(const std::string & frame_id, const ros::Time stamp, double max_dist,
                                        visualization_msgs::Marker& marker) const
{
  int maxXCell = getXNumCells();
  int maxYCell = getYNumCells();
  int maxZCell = getZNumCells();

  double * x_projection = new double[maxYCell*maxZCell];
  double * y_projection = new double[maxZCell*maxXCell];
  double * z_projection = new double[maxXCell*maxYCell];
  double initial_val = sqrt(INT_MAX);

  // Initialize
  for( int y = 0; y < maxYCell; y++ )
    for( int x = 0; x < maxXCell; x++ )
      z_projection[x+y*maxXCell] = initial_val;

  for( int z = 0; z < maxZCell; z++ )
    for( int y = 0; y < maxYCell; y++ )
      x_projection[y+z*maxYCell] = initial_val;

  for( int z = 0; z < maxZCell; z++ )
    for( int x = 0; x < maxXCell; x++ )
      y_projection[x+z*maxXCell] = initial_val;

  // Calculate projections
  for( int z = 0; z < maxZCell; z++ ) {
    for( int y = 0; y < maxYCell; y++ ) {
      for( int x = 0; x < maxXCell; x++ ) {
        double dist = getDistanceFromCell(x,y,z);
        z_projection[x+y*maxXCell] = std::min( dist, z_projection[x+y*maxXCell]);
        x_projection[y+z*maxYCell] = std::min( dist, x_projection[y+z*maxYCell]);
        y_projection[x+z*maxXCell] = std::min( dist, y_projection[x+z*maxXCell]);
      }
    }
  }

  // Make markers
  marker.points.clear();
  marker.header.frame_id = frame_id;
  marker.header.stamp = stamp;
  marker.ns = "distance_field_projection_plane";
  marker.id = 1;
  marker.type = visualization_msgs::Marker::CUBE_LIST;
  marker.action = visualization_msgs::Marker::MODIFY;
  marker.scale.x = getResolution();
  marker.scale.y = getResolution();
  marker.scale.z = getResolution();
  marker.color.a = 1.0;
  //marker.lifetime = ros::Duration(30.0);

  int x, y, z;
  int index = 0;
  marker.points.resize(maxXCell*maxYCell + maxYCell*maxZCell + maxZCell*maxXCell);
  marker.colors.resize(maxXCell*maxYCell + maxYCell*maxZCell + maxZCell*maxXCell);

  z = 0;
  for( y = 0; y < maxYCell; y++ ) {
    for( x = 0; x < maxXCell; x++ ) {
      double dist = z_projection[x+y*maxXCell];
      setPoint( x, y, z, dist, marker.points[index], marker.colors[index], max_dist );
      index++;
    }
  }

  x = 0;
  for( z = 0; z < maxZCell; z++ ) {
    for( y = 0; y < maxYCell; y++ ) {
      double dist = x_projection[y+z*maxYCell];
      setPoint( x, y, z, dist, marker.points[index], marker.colors[index], max_dist );
      index++;
    }
  }

  y = 0;
  for( z = 0; z < maxZCell; z++ ) {
    for( x = 0; x < maxXCell; x++ ) {
      double dist = y_projection[x+z*maxXCell];
      setPoint( x, y, z, dist, marker.points[index], marker.colors[index], max_dist );
      index++;
    }
  }

  if( x_projection)
    delete[] x_projection;
  if( y_projection )
    delete[] y_projection;
  if( z_projection )
    delete[] z_projection;
}



}
