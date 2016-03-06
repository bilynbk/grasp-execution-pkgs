#include <grasp_execution_msgs/GraspAction.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>

#include <arm_components_name_manager/ArmComponentsNameManager.h>

#include "SimpleGraspGenerator.h"
#include <object_msgs/Object.h>
#include <object_msgs/ObjectInfo.h>

bool getObjectInfo(const std::string& object_name, const std::string& REQUEST_OBJECTS_TOPIC, object_msgs::Object& obj)
{
	ros::NodeHandle n;
	ros::ServiceClient client = n.serviceClient<object_msgs::ObjectInfo>(REQUEST_OBJECTS_TOPIC);
	object_msgs::ObjectInfo srv;
	srv.request.name = object_name;
    srv.request.get_geometry=false;

	if (client.call(srv) && srv.response.success)
	{
		ROS_INFO("Result:");
		std::cout<<srv.response<<std::endl;
	}
	else
	{
		ROS_ERROR("Failed to call service %s, success flag: %i",REQUEST_OBJECTS_TOPIC.c_str(),srv.response.success);
		return false;
	}
    obj=srv.response.object;
    return true;
}

int main(int argc, char** argv)
{
    ros::init(argc, argv, "grasp_action_client");

    ros::NodeHandle priv("~");

    if (!priv.hasParam("object_name"))
    {
        ROS_ERROR("Object name required!");
        return 0;
    } 
    std::string OBJECT_NAME;
	priv.param<std::string>("object_name", OBJECT_NAME, OBJECT_NAME);

	std::string REQUEST_OBJECTS_TOPIC="world/request_object";
	priv.param<std::string>("request_object_service_topic", REQUEST_OBJECTS_TOPIC, REQUEST_OBJECTS_TOPIC);

    std::string ROBOT_NAMESPACE;
    if (!priv.hasParam("robot_namespace"))
    {
        ROS_ERROR("Node requires private parameter 'robot_namespace'");
        return 1;
    }
	priv.param<std::string>("robot_namespace", ROBOT_NAMESPACE, ROBOT_NAMESPACE);

    arm_components_name_manager::ArmComponentsNameManager jointsManager(ROBOT_NAMESPACE, false);
    double maxWait=5;
    ROS_INFO("Waiting for joint info parameters to be loaded...");
    jointsManager.waitToLoadParameters(1,maxWait); 
    ROS_INFO("Parameters loaded.");

    std::vector<std::string> gripperJoints = jointsManager.getGripperJoints();
    for (int i=0; i<gripperJoints.size(); ++i) ROS_INFO_STREAM("Gripper "<<i<<": "<<gripperJoints[i]);

    std::string arm_base_frame = jointsManager.getArmLinks().front();
 
    bool GRASPING=true;
	priv.param<bool>("grasping_action", GRASPING, GRASPING);
    
    double OPEN_ANGLES=0.05;
	priv.param<double>("open_angles", OPEN_ANGLES, OPEN_ANGLES);
    double CLOSE_ANGLES=0.7;
	priv.param<double>("close_angles", CLOSE_ANGLES, CLOSE_ANGLES);

    std::string GRASP_ACTION_TOPIC = "/grasp_action";
	priv.param<std::string>("grasp_action_topic", GRASP_ACTION_TOPIC, GRASP_ACTION_TOPIC);

    double POSE_ABOVE;
	priv.param<double>("pose_above_object", POSE_ABOVE, POSE_ABOVE);
    double POSE_X;
	priv.param<double>("x_from_object", POSE_X, POSE_X);
    double POSE_Y;
	priv.param<double>("y_from_object", POSE_Y, POSE_Y);
        
    object_msgs::Object obj;
   
    if (!getObjectInfo(OBJECT_NAME, REQUEST_OBJECTS_TOPIC, obj))
    {
        ROS_ERROR("Could not get info of object %s", OBJECT_NAME.c_str());
        return 0;
    }

    if (obj.primitive_poses.size()!=1)
    {
        ROS_ERROR_STREAM("Object "<<OBJECT_NAME<<" has more or less than one primitive: "
            << obj.primitive_poses.size()<<". This is not supported in simple grasp generator.");
        return 0;
    }

    geometry_msgs::PoseStamped obj_pose;
    obj_pose.pose = obj.primitive_poses[0];
    obj_pose.header = obj.header;

    std::string object_frame_id = obj.name;

    grasp_execution::SimpleGraspGenerator graspGen;
        
    manipulation_msgs::Grasp mgrasp;

    bool genGraspSuccess = grasp_execution::SimpleGraspGenerator::generateSimpleGraspFromTop(
        gripperJoints,
        arm_base_frame,
        "TestGrasp",
        OBJECT_NAME,        
        obj_pose,
        object_frame_id,
        POSE_ABOVE, POSE_X, POSE_Y,
        OPEN_ANGLES, CLOSE_ANGLES,
        mgrasp);

    if (!genGraspSuccess)
    {
        ROS_ERROR("Could not generate grasp");
        return 0;
    }

    ROS_INFO_STREAM("generated manipulation_msgs::Grasp: "<<std::endl<<mgrasp);

    ROS_INFO("END TEST");
    return 0;

    // create the action client
    // true causes the client to spin its own thread
    actionlib::SimpleActionClient<grasp_execution_msgs::GraspAction> ac(GRASP_ACTION_TOPIC, true);

    ROS_INFO("Waiting for action server to start: %s", GRASP_ACTION_TOPIC.c_str());
    // wait for the action server to start
    ac.waitForServer(); //will wait for infinite time
    ROS_INFO("Action server started.");


    ROS_INFO("Now constructing goal");
    /*
    ROS_INFO("Now sending goal");
    ac.sendGoal(goal);

    //wait for the action to return
    bool finished_before_timeout = ac.waitForResult(ros::Duration(15.0));

    if (finished_before_timeout)
    {
        actionlib::SimpleClientGoalState state = ac.getState();
        ROS_INFO("Action finished: %s",state.toString().c_str());
    }
    else
        ROS_INFO("Action did not finish before the time out.");

    return 0;*/
}
