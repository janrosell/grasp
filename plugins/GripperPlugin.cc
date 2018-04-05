/*!
    \file plugins/GripperPlugin.cc
    \brief Gripper Gazebo plugin

    TODO

    \author João Borrego : jsbruglie
*/

#include "GripperPlugin.hh"

namespace gazebo {


/// \brief Class for private camera utils plugin data.
class GripperPluginPrivate
{
    /// Gazebo transport node
    public: transport::NodePtr node;
    /// Camera utils topic subscriber
    public: transport::SubscriberPtr sub;
    /// Camera utils topic publisher
    public: transport::PublisherPtr pub;

    /// Mutex for safe data access
    public: std::mutex mutex;
};

// Register this plugin with the simulator
GZ_REGISTER_MODEL_PLUGIN(GripperPlugin)

/////////////////////////////////////////////////
GripperPlugin::GripperPlugin() : ModelPlugin(),
    data_ptr(new GripperPluginPrivate)
{
    gzmsg << "[GripperPlugin] Loaded gripper plugin." << std::endl;
}

/////////////////////////////////////////////////
GripperPlugin::~GripperPlugin()
{
    gzmsg << "[GripperPlugin] Unloaded gripper plugin." << std::endl;
}

/////////////////////////////////////////////////
void GripperPlugin::Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
{
    // Validate model to which plugin is attached
    if (_model->GetJointCount() != 0)
    {
        this->model = _model;
    }
    else
    {
        gzerr << "[GripperPlugin] Invalid model." << std::endl;
        return;
    }

    // Extract parameters from SDF element

    // Reference links and joints
    if (_sdf->HasElement(PARAM_BASE_LINK) &&
        _sdf->HasElement(PARAM_LEFT_LINK) &&
        _sdf->HasElement(PARAM_RIGHT_LINK) &&
        _sdf->HasElement(PARAM_LEFT_JOINT) &&
        _sdf->HasElement(PARAM_RIGHT_JOINT) &&
        _sdf->HasElement(PARAM_VIRTUAL_JOINTS))
    {
        std::string base_link_name = _sdf->Get<std::string>(PARAM_BASE_LINK);
        std::string left_link_name = _sdf->Get<std::string>(PARAM_LEFT_LINK);
        std::string right_link_name = _sdf->Get<std::string>(PARAM_RIGHT_LINK);
        std::string left_joint_name = _sdf->Get<std::string>(PARAM_LEFT_JOINT);
        std::string right_joint_name = _sdf->Get<std::string>(PARAM_RIGHT_JOINT);
        std::string virtual_joints_str = _sdf->Get<std::string>(PARAM_VIRTUAL_JOINTS);

        // Convert string to array of strings
        std::vector<std::string> virtual_joint_names;
        boost::split(virtual_joint_names, virtual_joints_str,
            boost::is_any_of(" "), boost::token_compress_on);
        int num_virtual_joints = virtual_joint_names.size();

        this->base_link = _model->GetLink(base_link_name);
        this->left_link = _model->GetLink(left_link_name);
        this->right_link = _model->GetLink(right_link_name);
        this->left_joint = _model->GetJoint(left_joint_name);
        this->right_joint = _model->GetJoint(right_joint_name);
        for (auto joint_name : virtual_joint_names) {
            this->virtual_joints.push_back(_model->GetJoint(joint_name));
        }

        if (!this->base_link || !this->left_link || !this->right_link ||
            !this->left_joint || !this->right_joint ||
            (this->virtual_joints.size() != num_virtual_joints) )
        {
            gzerr << "[GripperPlugin] Invalid reference link or joint specified." << std::endl;
            return;
        }
    }
    else
    {
        gzerr << "[GripperPlugin] Unspecified reference link or joint." << std::endl;
        return;
    }

    // Enable/disable gravity
    if (_sdf->HasElement(PARAM_GRAVITY))
    {
        bool gravity_enabled = _sdf->Get<bool>(PARAM_GRAVITY);
        this->model->SetGravityMode(gravity_enabled);
    }

    // Connect to world update event
    this->update_connection = event::Events::ConnectWorldUpdateBegin(
        std::bind(&GripperPlugin::onUpdate, this));

    // Setup transport node
    this->data_ptr->node = transport::NodePtr(new transport::Node());
    this->data_ptr->node->Init();
    // Subcribe to the monitored requests topic
    this->data_ptr->sub = this->data_ptr->node->Subscribe(REQUEST_TOPIC,
        &GripperPlugin::onRequest, this);
}

/////////////////////////////////////////////////
void GripperPlugin::onUpdate()
{
    if (this->update_pose) {
        setPose(this->new_pose);
        this->update_pose = false;
    }
    if (this->update_velocity) {
        setVelocity(this->new_velocity);
        this->update_velocity = false;
    }
    if (this->new_open != this->open) {
        (this->new_open)? openGripper() : closeGripper();
        this->open = this->new_open;
    }
}

/////////////////////////////////////////////////
void GripperPlugin::onRequest(GripperMsgPtr &_msg)
{
    ignition::math::Pose3d pose;
    ignition::math::Vector3d velocity;

    // Handle change pose request
    if (_msg->has_pose()) {
        this->new_pose = msgs::ConvertIgn(_msg->pose());
        this->update_pose = true;
    }
    // Handle change velocity request
    if (_msg->has_velocity()) {
        this->new_velocity = msgs::ConvertIgn(_msg->velocity());
        this->update_velocity = true;
    }
    // Handle open/close gripper request
    if (_msg->has_open()) {
        this->new_open = _msg->open();
    }
}

/////////////////////////////////////////////////
void GripperPlugin::setPose(ignition::math::Pose3d & _pose)
{
    gzmsg << "Received change pose request" << std::endl;

    ignition::math::Vector3d pos = _pose.Pos();
    ignition::math::Quaterniond rot = _pose.Rot();
    this->virtual_joints.at(0)->SetPosition(0, pos.X());
    this->virtual_joints.at(1)->SetPosition(0, pos.Y());
    this->virtual_joints.at(2)->SetPosition(0, pos.Z());
    this->virtual_joints.at(3)->SetPosition(0, rot.Roll());
    this->virtual_joints.at(4)->SetPosition(0, rot.Pitch());
    this->virtual_joints.at(5)->SetPosition(0, rot.Yaw());
}

/////////////////////////////////////////////////
void GripperPlugin::setVelocity(ignition::math::Vector3d & _velocity)
{
    this->virtual_joints.at(0)->SetVelocity(0, _velocity.X());
    this->virtual_joints.at(1)->SetVelocity(0, _velocity.Y());
    this->virtual_joints.at(2)->SetVelocity(0, _velocity.Z());
}

/////////////////////////////////////////////////
void GripperPlugin::openGripper()
{
    this->left_joint->SetPosition(0, this->left_joint->UpperLimit());
    this->right_joint->SetPosition(0, this->right_joint->LowerLimit());
}

/////////////////////////////////////////////////
void GripperPlugin::closeGripper()
{
    this->left_joint->SetPosition(0, this->left_joint->LowerLimit());
    this->right_joint->SetPosition(0, this->right_joint->UpperLimit());
}

}
