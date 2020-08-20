/** Simple 2D robot simulator, with image-based debug info. */
#include "vec4.h" ///< for 3D vector class vec3 (like GLSL), with length, dot product, etc.

class mobile_robot;

/** This class watches the robots as they drive around. */
class robot_watcher {
public:
	virtual void update(const mobile_robot &robot) =0;
};

/** Return the angle, in degrees, that this vector is pointing. */
float angle_degrees(vec3 relative) {
	float angle_rad=atan2(relative.y,relative.x);
	return angle_rad*180.0/M_PI;
}

/** An abstract representation of a driving robot,
   like a car, Roomba, or LAYLA. */
class mobile_robot {
	// Each robot has a list of watchers
	std::vector<robot_watcher *> watchers;
public:
	
	vec3 pos; ///< Location of robot's center in arena, in meters.  XY is robot's driving plane.  Z is up.
	float angle; ///< Angle robot is facing, in degrees: 0 == facing +X.  90 == facing +Y
	float mileage; ///< Total drive distance
	
	mobile_robot(const vec3 &startpos=vec3(0.0,0.0,0.0),float startang=0.0) 
	{
		pos=startpos;
		angle=startang;
		mileage=0.0;
	}
	
	/// Add this watcher for this robot
	void add_watcher(robot_watcher *w) {
		watchers.push_back(w);
	}
	
	/// Notify each watcher of this robot's current position
	void update_watchers(void) const {
		for (robot_watcher *w:watchers) w->update(*this);
	}
	
	// Print the robot's location to the console
	void print(void) const {
		printf("Robot at %.1f, %.1f heading %.0f\n",
			pos.x, pos.y, angle);
	}
	
	/***
	 Convert from robot coordinates to world coordinates:
	 Return the point this far along the robot's forward axis (drive direction)
	 plus (optionally) this far along the robot's right side axis,
	 after (optionally) turning the robot by spin_deg degrees.
	*/
	vec3 get_forward(float forward,float right=0.0, float spin_deg=0.0) const {
		float rads=(angle+spin_deg)*3.141592/180.0;
		float c=cos(rads), s=sin(rads);
		return pos + vec3(
			c*forward - s*right,
			s*forward + c*right,
			0.0
		);
	}
	
	/// Drive forward this far
	virtual void forward(float distance);
	
	/// Turn left by this many degrees (must have angle_deg between -360 and 360)
	virtual void left(float angle_deg);
};

/// Drive forward this far
void mobile_robot::forward(float distance)
{
	float step=0.14;
	for (float d=0.0;d<distance;d+=step) {
		pos = get_forward(step);
		mileage += step;
		update_watchers();
	}
}

/// Utility function to wrap degrees around to 0-360
void reduce360(float &angle)
{
	while (angle<0.0f) angle+=360.0f;
	while (angle>=360.0f) angle-=360.0f;
}

/// Turn left by this many degrees
void mobile_robot::left(float angle_deg)
{
	int direction=+1;
	if (angle_deg<0.0) {
		angle_deg=-angle_deg; 
		direction=-1;
	}
	reduce360(angle_deg);
	float step=3.8;
	for (float a=0.0;a<angle_deg;a+=step) {
		angle += direction*step;
		reduce360(angle);
		update_watchers();
	}
}


/** Print the robot's location as it drives. */
class robot_print_watcher {
public:
	virtual void update(const mobile_robot &robot) {
		robot.print(); // dump position, for debugging
	}
};


class robot_obstacles {
public:
	/// Return true if this area is obstructed
	virtual bool is_obstacle(const vec3 &position) const =0;
};

/** Make sure the robot doesn't bash into any obstacles as it drives. */
class obstacle_watcher : public robot_watcher {
public:
	robot_obstacles &obstacles;
	obstacle_watcher(robot_obstacles &o) :obstacles(o) {}
	
	virtual void update(const mobile_robot &robot) {
		if (obstacles.is_obstacle(robot.pos)) {
			printf("Dang, we just rammed an obstacle!\n");
			throw robot;
		}
	}
};



#include <fstream> /* for ofstream */
/// Watcher that draws a color raster image as the robot moves
class drawing_watcher : public robot_watcher {
public:
	const static int wid=1024, ht=1024; ///< Size of image in pixels
	float size; ///< Width of arena in meters
	float pixels_from_meters;
	float meters_from_pixels;
	int edge; ///< pixels around edge of image
	int image[wid*ht]; ///< Color pixels of image
	
	drawing_watcher(float size_, int background_color=0) 
	{
		size=size_;
		edge=50;
		pixels_from_meters=(wid-edge*2)/size;
		meters_from_pixels=1.0/pixels_from_meters;
		
		for (int y=0;y<ht;y++) 
		for (int x=0;x<wid;x++)
		{
			// Draw 1 meter grid lines:
			float fx=meters_from_pixels*(x-edge);
                        float fy=meters_from_pixels*(y-edge);
			float linewidth=meters_from_pixels*1.5;
			if (fmod(fx+100,1.0)<linewidth || fmod(fy+100,1.0)<linewidth)
				image[y*wid+x]=0x404040; // gray lines
			else
				image[y*wid+x] = background_color;
		}
	}
	
	// Draw obstacles with this color
	void draw_obstacles(const robot_obstacles &obstacles,int color) {
		for (int y=0;y<ht;y++) 
		for (int x=0;x<wid;x++)
		{
			float fx=meters_from_pixels*(x-edge);
			float fy=meters_from_pixels*(y-edge);
			if (obstacles.is_obstacle(vec3(fx,fy,0.0)))
				image[y*wid+x] = color;
		}
	}
	
	// Draw a dot at this location
	void dot(const vec3 &loc,int color)
	{
		// Scale arena coordinates to pixels
		int x=pixels_from_meters*loc.x+edge;
		if (x<0 || x>=wid) return;
		
		int y=pixels_from_meters*loc.y+edge; 
		if (y<0 || y>=ht) return;
		
		image[y*wid+x] = color;
	}
	
	// Draw the location and orientation of the robot at this location
	virtual void update(const mobile_robot &robot) {
		float pixelstep = 0.5*meters_from_pixels;
		for (float line=0.0; line<=0.2; line+=pixelstep)
			dot(robot.get_forward(line),0x00ff00); // robot centerline in green

		for (int side=-1;side<=+1;side+=2) // left and right side wheels
			dot(robot.get_forward(0.0, side*0.15), 0x4040ff); // wheels in blue
	}
	
	void write(const char *filename="out.ppm") {
		std::ofstream out(filename,std::ios_base::binary);
		out<<"P6\n"
		   <<wid<<" "<<ht<<"\n"
		   <<"255\n";
		for (int y=0;y<ht;y++) 
		for (int x=0;x<wid;x++)
		{
			unsigned char r,g,b;
			int i=image[(ht-1-y)*wid+x]; // file is stored Y axis down
			r=(unsigned char)(0xff&(i>>16)); // red channel in high bits
			g=(unsigned char)(0xff&(i>>8));
			b=(unsigned char)(0xff&(i>>0)); // blue channel in low bits
			out<<r<<g<<b;
		}
	}
};


class sinwave_obstacles : public robot_obstacles {
public:
	vec3 scales;
	sinwave_obstacles(const vec3 &scales_=vec3(1,1,1)) :scales(scales_) 
	{}

	/// Return true if this area is obstructed
	virtual bool is_obstacle(const vec3 &position) const {
		float px=scales.x*position.x-1.5;
		float py=scales.y*position.y+0.5;
		float x=px*+3.0 + py*1.0;
		float y=px*-1.0 + py*3.0;
		return (sin(0.5*x)+sin(2.3*x)+sin(y))>1.6;
	}
	
};

class LIDAR_watcher : public robot_watcher {
public:
	drawing_watcher &image;
	robot_obstacles &obstacles;
	LIDAR_watcher(drawing_watcher &image_, robot_obstacles &obstacles_)
		:image(image_), obstacles(obstacles_) {}

	void update(const mobile_robot &robot) {
		float pixelstep = 0.7*image.meters_from_pixels;
		for (float lidar=0.0; lidar<=10.0; lidar+=pixelstep)
		{
			vec3 p=robot.get_forward(lidar);
			if (obstacles.is_obstacle(p))
			{
				image.dot(p,0xff8080); // bright red dot at end of beam
				break;
			}
			else
				image.dot(p,0x800000); // dim red along beam
		}
	}
};

class limiter_watcher : public robot_watcher {
public:
	float drive_limit;
	limiter_watcher(float limit) :drive_limit(limit) {}
	void update(const mobile_robot &robot) {
		if (robot.mileage>drive_limit) {
			printf("Robot has reached mileage limit %.0f meters\n",robot.mileage);
			throw robot;
		}
	}
};

