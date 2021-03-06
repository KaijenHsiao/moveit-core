cmake_minimum_required(VERSION 2.8.3)
project(moveit_core)

find_package(Eigen REQUIRED)
include_directories(SYSTEM ${EIGEN_INCLUDE_DIRS})

find_package(Boost REQUIRED system filesystem date_time thread)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

find_package(PkgConfig REQUIRED)

pkg_check_modules(FCL REQUIRED fcl)
include_directories(${FCL_INCLUDE_DIRS})
link_directories(${FCL_LIBRARY_DIRS})

pkg_check_modules(OCTOMAP REQUIRED octomap)
include_directories(${OCTOMAP_INCLUDE_DIRS})
link_directories(${OCTOMAP_LIBRARY_DIRS})

find_package(catkin COMPONENTS
  moveit_msgs
  resource_retriever
  urdfdom
  srdfdom
  kdl_parser
  geometric_shapes
  shape_tools
  eigen_stl_containers 
  eigen_conversions
  random_numbers
  console_bridge
  visualization_msgs
  rostime REQUIRED)
include_directories(${catkin_INCLUDE_DIRS})
link_directories(${catkin_LIBRARY_DIRS})

catkin_package(
  LIBRARIES
    moveit_kinematic_model
    moveit_kinematic_state
    moveit_collision_detection
    moveit_collision_detection_fcl
    moveit_kinematic_constraints
    moveit_planning_scene
    moveit_constraint_samplers
    moveit_planning_request_adapter
    moveit_trajectory_processing
    moveit_distance_field
    moveit_collision_distance_field
    moveit_kinematics_metrics
    moveit_dynamics_solver
  DEPENDS
    Boost Eigen
    geometric_shapes
    shape_tools
    eigen_stl_containers
    eigen_conversions
    random_numbers
    console_bridge
    moveit_msgs)

include_directories(kinematics_base/include)
include_directories(kinematic_model/include)
include_directories(kinematic_state/include)
include_directories(collision_detection/include)
include_directories(collision_detection_fcl/include)
include_directories(kinematic_constraints/include)
include_directories(planning_scene/include)
include_directories(constraint_samplers/include)
include_directories(planning_interface/include)
include_directories(planning_request_adapter/include)
include_directories(trajectory_processing/include)
include_directories(distance_field/include)
include_directories(collision_distance_field/include)
include_directories(kinematics_metrics/include)
include_directories(dynamics_solver/include)

add_subdirectory(kinematics_base)
add_subdirectory(kinematic_model)
add_subdirectory(kinematic_state)
add_subdirectory(collision_detection)
add_subdirectory(collision_detection_fcl)
add_subdirectory(kinematic_constraints)
add_subdirectory(planning_scene)
add_subdirectory(constraint_samplers)
add_subdirectory(planning_interface)
add_subdirectory(planning_request_adapter)
add_subdirectory(trajectory_processing)
add_subdirectory(distance_field)
add_subdirectory(collision_distance_field)
add_subdirectory(kinematics_metrics)
add_subdirectory(dynamics_solver)

#add_subdirectory(kinematics_cache)
#add_subdirectory(kinematics_constraint_aware)

#add_subdirectory(kinematics_planner)
