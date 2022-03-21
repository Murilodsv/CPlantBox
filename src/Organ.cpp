// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#include "Organ.h"

#include "Organism.h"
#include "Plant.h"
#include <iostream>

#include "organparameter.h"

namespace CPlantBox {

/**
 * Constructs an organ from given data.
 * The organ tree must be created, @see Organ::setPlant, Organ::setParent, Organ::addChild
 * Organ geometry must be created, @see Organ::addNode, ensure that this->getNodeId(0) == parent->getNodeId(pni)
 *
 * @param id        the organ's unique id (@see Organ::getId)
 * @param param     the organs parameters set, ownership transfers to the organ
 * @param alive     indicates if the organ is alive (@see Organ::isAlive)
 * @param active    indicates if the organ is active (@see Organ::isActive)
 * @param age       the current age of the organ (@see Organ::getAge)
 * @param length    the current length of the organ (@see Organ::getLength)
 * @param iHeading TODO
 * @param pni
 * @param moved     indicates if nodes were moved in the previous time step (default = false)
 * @param oldNON    the number of nodes of the previous time step (default = 0)
 */
Organ::Organ(int id, std::shared_ptr<const OrganSpecificParameter> param, bool alive, bool active,
		double age, double length, Matrix3d iHeading, int pni, bool moved, int oldNON)
:iHeading(iHeading), parentNI(pni), plant(), parent(), id(id), param_(param), alive(alive), active(active), age(age),
 length(length), moved(moved), oldNumberOfNodes(oldNON)
{ }

/**
 * The constructor is used for simulation.
 * The organ parameters are chosen from random distributions within the the OrganTypeParameter class.
 * The next organ id is retrieved from the plant,
 * and the organ starts growing after a delay (starts with age = -delay).
 *
 * @param plant     the plant the new organ will be part of
 * @param parent    the parent organ, equals nullptr if there is no parent
 * @param ot        organ type
 * @param st        sub type of the organ type, e.g. different Organ types
 * @param delay     time delay in days when the organ will start to grow
 * @param iHeading TODO
 * @param pni
 */
Organ::Organ(std::shared_ptr<Organism> plant, std::shared_ptr<Organ> parent, int ot, int st, double delay,
		Matrix3d iHeading, int pni)
:iHeading(iHeading), parentNI(pni), plant(plant), parent(parent), id(plant->getOrganIndex()),
  param_(plant->getOrganRandomParameter(ot, st)->realize()), /* Organ parameters are diced in the getOrganRandomParameter class */
  age(-delay)
{ }

/*
 * Deep copies this organ into the new plant @param plant.
 * All children are deep copied, plant and parent pointers are updated.
 *
 * @param plant     the plant the copied organ will be part of
 * @return          the newly created copy (ownership is passed)
 */
std::shared_ptr<Organ> Organ::copy(std::shared_ptr<Organism>  p)
{
	auto o = std::make_shared<Organ>(*this); // shallow copy
	o->parent = std::weak_ptr<Organ>();
	o->plant = p;
	o->param_ = std::make_shared<OrganSpecificParameter>(*param_); // copy parameters
	for (size_t i=0; i< children.size(); i++) {
		o->children[i] = children[i]->copy(p); // copy lateral
		o->children[i]->setParent(o);
	}
	return o;
}

/**
 * @param realized	FALSE:	get theoretical organ length, INdependent from spatial resolution (dx() and dxMin())
 *					TRUE:	get realized organ length, dependent from spatial resolution (dx() and dxMin())
 *					DEFAULT = TRUE
 * @return 			The chosen type of organ length (realized or theoretical).
 */
double Organ::getLength(bool realized) const
{
	if (realized) {
		return length - this->epsilonDx;
	} else {
		return length;
	}
}

/**
 * @return the organs length from start node up to the node with index @param i.
 */
double Organ::getLength(int i) const
{
	double l = 0.; // length until node i
	for (int j = 0; j<i; j++) {
		if(this->getParameter("organType")==Organism::ot_root){
			l += nodes.at(j+1).minus(nodes.at(j)).length(); // relative length equals absolute length
		}else{
			// relative length equals absolute length during growth
			// for leaves and stem
			l += nodes.at(j+1).length(); 
		}
	}
	return l;
}

/**
 * @return The organ type, which is a coarse classification of the organs,
 * for a string representation see see Organism::organTypeNames
 *
 * Currently there are: ot_organ (for unspecified organs) = 0, ot_seed = 1, ot_root = 2, ot_stem = 3, and ot_leaf = 4.
 * There can be different classes with the same organ type.
 */
int Organ::organType() const
{
	return Organism::ot_organ;
}

/**
 * @return The organ type parameter is retrieved from the plant organism.
 * The Organism class manages all organs type parameters.
 */
std::shared_ptr<OrganRandomParameter> Organ::getOrganRandomParameter() const
{
	return plant.lock()->getOrganRandomParameter(this->organType(), param_->subType);
}

/**
 * Simulates the development of the organ in a time span of @param dt days.
 *
 * @param dt        time step [day]
 * @param verbose   turns console output on or off
 */
void Organ::simulate(double dt, bool verbose)
{
	// store information of this time step
	oldNumberOfNodes = nodes.size();
	moved = false;

	// if the organ is alive, manage children
	if (alive) {
		age += dt;
		for (auto& c : children)  {
			c->simulate(dt, verbose);
		}
	}
}

/**
 *
 */
std::shared_ptr<Plant> Organ::getPlant() const
{
	return std::dynamic_pointer_cast<Plant>(plant.lock());
}

/*
 * Adds a subsequent organ (e.g. a lateral Organ)
 *
 * @param c     the organ to add (ownership is passed)
 */
void Organ::addChild(std::shared_ptr<Organ> c)
{
	c->setParent(shared_from_this());
	children.push_back(c);
}

/**
 * Adds a node to the organ.
 *
 * For simplicity nodes can not be deleted, organs can only become deactivated or die
 *
 * @param n        new node
 * @param id       global node index
 * @param t        exact creation time of the node
 * @param index	   position were new node is to be added
 * @param shift	   do we need to shift the nodes? (i.e., is the new node inserted between existing nodes because of internodal growth?)
 */
void Organ::addNode(Vector3d n, int id, double t, size_t index, bool shift)
{
	if(!shift){//node added at the end of organ										
		nodes.push_back(n); // node
		nodeIds.push_back(id); //unique id
		nodeCTs.push_back(t); // exact creation time
	}
	else{//could be quite slow  to insert, but we won t have that many (node-)tillers (?) 
		nodes.insert(nodes.begin() + index, n);//add the node at index
		//add a global index. 
		//no need for the nodes to keep the same global index and makes the update of the nodes position for MappedPlant object more simple)
		nodeIds.push_back(id);  
		nodeCTs.insert(nodeCTs.begin() + index-1, t);
		for(auto kid : children){//if carries children after the added node, update their "parent node index"
			if(kid->parentNI >= index-1){
				kid->moveOrigin(kid->parentNI + 1);
				}
			
		}
		
	}
}

/**
 * change idx of node linking to parent organ (in case of internodal growth)
 * @see Organ::addNode
 * @param idx      new idx
 */
void Organ::moveOrigin(int idx)
{
	this->parentNI = idx;
	nodeIds.at(0) = getParent()->getNodeId(idx);
	
}

/**
 * Adds the node with the next global index to the Organ
 *
 * For simplicity nodes can not be deleted, organs can only become deactivated or die
 *
 * @param n        the new node
 * @param t        exact creation time of the node
 * @param index	   position were new node is to be added
 * @param shift	   do we need to shift the nodes? (i.e., is the new node inserted between existing nodes because of internodal growth?)
 */
void Organ::addNode(Vector3d n, double t, size_t index, bool shift)
{
	addNode(n,plant.lock()->getNodeIndex(),t, index, shift);
}

/**
 * By default the organ is represented by a polyline,
 * i.e. the segments of the nodes {n1, n2, n3, n4}, are { [i1,i2], [i2,i3], [i3,i4] }, where i1-i4 are node indices.
 *
 * @return A vector of line segments, where each line segment is described as two global node indices.
 * If there are less than two nodes an empty vector is returned.
 */
std::vector<Vector2i> Organ::getSegments() const
{
	if (this->nodes.size()>1) {
		std::vector<Vector2i> segs = std::vector<Vector2i>(nodes.size()-1);
		for (size_t i=0; i<nodes.size()-1; i++) {
			Vector2i s(getNodeId(i),getNodeId(i+1));
			segs[i] = s;
		}
		return segs;
	} else {
		return std::vector<Vector2i>(0);
	}
}

/**
 * returns the maximal axial resolution
 */
double Organ::dx() const
{
	return getOrganRandomParameter()->dx;
}

/**
 * returns the minimal axial resolution,
 * length overhead is stored in epsilon and realized in the next simulation step (see Organ::getEpsilon)
 */
double Organ::dxMin() const
{
	return getOrganRandomParameter()->dxMin;
}

/**
 * Returns the organs as sequential list, copies only organs with more than one node.
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default).
 *
 * @return A sequential list of organs. If there is less than one node,
 * or another organ type is expected, an empty vector is returned.
 */
std::vector<std::shared_ptr<Organ>> Organ::getOrgans(int ot)
{
	auto v = std::vector<std::shared_ptr<Organ>> ();
	this->getOrgans(ot, v);
	return v;
}

/**
 * Returns the organs as sequential list, copies only organs with more than one node.
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default).
 * @param v         vector of organs where the subtree is added,
 *                  only expected organ types with more than one nodes are added.
 */
void Organ::getOrgans(int ot, std::vector<std::shared_ptr<Organ>>& v)
{
	if (this->nodes.size()>1) {
		if ((ot<0) || (ot==this->organType())) {
			v.push_back(shared_from_this());
		}
	}
	// std::cout << "Organ::getOrgans recursive: number of children " <<  this->children.size() << "\n" << std::flush;
	for (const auto& c : this->children) {
		c->getOrgans(ot,v);
	}
}

/**
 * @return The number of emerged laterals (i.e. number of children with age>0)
 * @see Organ::getNumberOfChildren
 * needed for the test files
 */
int Organ::getNumberOfLaterals() const {
	int nol = 0;
	for (auto& c : children)  {
		if (c->getAge()>0) { // born
			nol ++;
		}
	}
	return nol;
}

/**
 * Returns a single scalar parameter called @param name of the organ.
 * This method is for post processing, since it is flexible but slow.
 * Overwrite to add more parameters for specific organs.
 *
 * For OrganRandomParameters add '_mean', or '_dev',
 * to avoid naming conflicts with the organ specific parameters.
 *
 * @return The parameter value, if unknown NaN
 */
double Organ::getParameter(std::string name) const {
	// specific parameters
	if (name=="volume") { return param()->a*param()->a*M_PI*getLength(true); } // // Organ volume [cm^3]
	if (name=="surface") { return 2*param()->a*M_PI*getLength(true); }
	if (name=="type") { return this->param_->subType; }  // in CPlantBox the subType is often called just type
	if (name=="parentNI") { return parentNI; } // local parent node index where the lateral emerges
	if (name=="nob") { return param()->nob(); } // number of branches
	if (name=="r"){ return param()->r; }  // initial growth rate [cm day-1]
	if (name=="theta") { return param()->theta; } // angle between Organ and parent Organ [rad]
	if (name=="lnMean") { // mean lateral distance [cm]
		auto& v =param()->ln;
		return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
	}
	if (name=="lnDev") { // standard deviation of lateral distance [cm]
		auto& v =param()->ln;
		double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
		double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
		return std::sqrt(sq_sum / v.size() - mean * mean);
	}
	if (name=="subType") { return this->param_->subType; }
    if (name=="rlt") { return param()->rlt; } // Organ life time [day]
	if (name=="k") { return param()->getK(); }; // maximal Organ length [cm]
	if (name=="lb") { return param()->lb; } // basal zone [cm]
	if (name=="la") { return param()->la; } // apical zone [cm]
	if (name=="a") { return param_->a; } // Organ radius [cm]
	if (name=="radius") { return this->param_->a; } // Organ radius [cm]
	if (name=="diameter") { return 2.*this->param_->a; } // Organ diameter [cm]
	// organ member variables
    if (name=="iHeadingX") { return iHeading.column(0).x; } // Organ initial heading x - coordinate [cm]
    if (name=="iHeadingY") { return iHeading.column(0).y; } // Organ initial heading y - coordinate [cm]
    if (name=="iHeadingZ") { return iHeading.column(0).z; } // Organ initial heading z - coordinate [cm]
    if (name=="parentNI") { return parentNI; } // local parent node index where the lateral emerges
    if (name=="parent-node") { // local parent node index for RSML (higher order Organs are missing the first node)
        if (this->parent.expired()) {
            return -1;
        }
        if (this->parent.lock()->organType()==Organism::ot_seed) { // if it is base Organ
    		return -1;
    	}
        auto p = this->parent.lock();
        if (p->parent.expired()) { // if parent is base Organ
            return parentNI;
        }
        if (p->parent.lock()->organType()==Organism::ot_seed){ // if parent is base Organ
            return parentNI;
        } else {
            return std::max(parentNI-1,0); // higher order Organs are missing the first node
            // TODO for 0 this can be negative... (belongs to other branch in rsml)
        }
    }
    // organ member functions
	if (name=="organType") { return this->organType(); }
    if (name=="numberOfChildren") { return children.size(); }
	if (name=="id") { return getId(); }
	if (name=="alive") { return isAlive(); }
	if (name=="active") { return isActive(); }
	if (name=="age") { return getAge(); }
    if (name=="length") { return getLength(true); } //realized organ length, dependent on dxMin and dx
	if (name=="lengthTh") { return getLength(false); } //theoratical organ length, dependent on dxMin and dx
    if (name=="numberOfNodes") { return getNumberOfNodes(); }
    if (name=="numberOfSegments") { return getNumberOfSegments(); }
    if (name=="hasMoved") { return hasMoved(); }
    if (name=="oldNumberOfNodes") { return getOldNumberOfNodes(); }
	if (name=="numberOfLaterals") { return getNumberOfLaterals(); }
    // further
    if (name=="creationTime") { return getNodeCT(0); }
	if (name=="order") { // count how often it is possible to move up
		int o = 0;
		auto p = shared_from_this();
		while ((!p->parent.expired()) && (p->parent.lock()->organType()!=Organism::ot_seed)) {
			o++;
			p = p->parent.lock(); // up the organ tree
		}
		return o;
	}
	if (name=="one") { return 1; } // e.g. for counting the organs
    return this->getOrganRandomParameter()->getParameter(name); // ask the random parameter
}

/**
 * Writes the organs RSML Organ tag, if it has more than one node.
 *
 * Called by Organism::getRSMLScene, not exposed to Python
 *
 * @param doc          the xml document (supplies factory functions)
 * @param parent       the parent xml element, where the organ's tag is added
 */
void Organ::writeRSML(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent) const
{
	if (this->nodes.size()>1) {
		int nn = plant.lock()->getRSMLSkip()+1;
		// organ
		// std::string name = getOrganTypeParameter()->name; // todo where to put it
		tinyxml2::XMLElement* organ = doc.NewElement("root"); // TODO use ot to fetch tag name?
		organ->SetAttribute("ID", id);
		// geometry
		tinyxml2::XMLElement* geometry = doc.NewElement("geometry");
		organ->InsertEndChild(geometry);
		tinyxml2::XMLElement* polyline = doc.NewElement("polyline");


		int o;
		if (this->parent.expired()) { // baseRoot = 0, others = 1
		    // std::cout << this->toString() << std::flush;
		    o = 0;
		} else {
		    if (this->parent.lock()->organType()==Organism::ot_seed) {
		        o = 0;
		    } else {
		        o = 1;
		    }
		}
		for (int i = o; i<getNumberOfNodes(); i+=nn) {
			auto n = getNode(i);
			tinyxml2::XMLElement* p = doc.NewElement("point");
			p->SetAttribute("x", float(n.x));
			p->SetAttribute("y", float(n.y));
			p->SetAttribute("z", float(n.z));
			polyline->InsertEndChild(p);
		}
		geometry->InsertEndChild(polyline);
		// properties
		tinyxml2::XMLElement* properties = doc.NewElement("properties");
		auto prop_names = plant.lock()->getRSMLProperties();
		for (const auto& pname : prop_names) {
			tinyxml2::XMLElement* p = doc.NewElement(pname.c_str());
			p->SetAttribute("value", float(this->getParameter(pname)));
			properties->InsertEndChild(p);
		}
		organ->InsertEndChild(properties);
		/* laterals Organs */
		for (size_t i = 0; i<children.size(); i+=nn) {
			children[i]->writeRSML(doc, organ);
		}
		// functions
		tinyxml2::XMLElement* fcts = doc.NewElement("functions");
		tinyxml2::XMLElement* fun1 = doc.NewElement("function");
		fun1->SetAttribute("domain","polyline");
		fun1->SetAttribute("name","node_creation_time");
		for (int i = o; i<getNumberOfNodes(); i+=nn) {
			double ct = getNodeCT(i);
			tinyxml2::XMLElement* p = doc.NewElement("sample");
			p->SetAttribute("value", ct);
			fun1->InsertEndChild(p);
		}
		tinyxml2::XMLElement* fun2 = doc.NewElement("function");
		fun2->SetAttribute("domain","polyline");
		fun2->SetAttribute("name","node_index");
		for (int i = o; i<getNumberOfNodes(); i+=nn) {
			int nid = getNodeId(i);
			tinyxml2::XMLElement* p = doc.NewElement("sample");
			p->SetAttribute("value", nid);
			fun2->InsertEndChild(p);
		}
		fcts->InsertEndChild(fun1);
		fcts->InsertEndChild(fun2);
		organ->InsertEndChild(fcts);
		parent->InsertEndChild(organ);
	}
}

/**
 * @return Quick info about the object for debugging,
 * additionally, use getParam()->toString() and getOrganRandomParameter()->toString() to obtain all information.
 */
std::string Organ::toString() const
{
	std::stringstream str;
	str << Organism::organTypeNames.at(this->organType()) << " #"<< getId() <<": sub type "<< param_->subType
					<< ", realized length " << getLength(true)
					<< " cm , theoretic length " << getLength(false) << " cm , age " << getAge()
    				<< " days, alive " << isAlive() << ", active " << isActive() << ", number of nodes " << this->getNumberOfNodes()
					<< ", with "<< children.size() << " children";
	return str.str();
}


/**
 * @param n      node index
 * @return Current absolute heading of the organ at node n, based on initial heading, or segment before
 * 
 */
Vector3d Organ::heading(int n) const
{
	if(n<0){n=nodes.size()-1 ;}
	if ((nodes.size()>1)&&(n>0)) {
		n = std::min(int(nodes.size()),n);
		Vector3d h;
		h = getNode(n).minus(getNode(n-1));//rel coordinates for leaf and stem
		h.normalize();
		return h;
	} else {
		return getiHeading();
	}
}



/**
 * Analytical creation (=emergence) time of a point along the already grown organ
 *
 * @param length   length along the organ, where the point is located [cm]
 * @param dt 	   current time step [day]
 * @return         the analytic time when this point was reached by the growing organ [day],
 * 				   if growth is impeded, the value is not exact, but approximated dependent on the temporal resolution.
 */
double Organ::calcCreationTime(double length, double dt)
{
    assert(length >= 0 && "Organ::getCreationTime() negative length");
    double age_ = calcAge(std::max(length,0.)); // organ age as if grown unimpeded (lower than real age)
    
	assert(age_ >= 0 && "Organ::getCreationTime() negative age");
	double a = std::max(age_, age-dt /*old age*/);
    a = std::min(a, age); // a in [age-dt, age]
    return a+nodeCTs[0];
}
/**
 * Analytical length of the organ at a given age
 *
 * @param age          age of the organ [day]
 */
double Organ::calcLength(double age)
{
	assert(age>=0  && "Organ::calcLength() negative Organ age");
	return getF_gf()->getLength(age,getParameter("r"),getParameter("k"),shared_from_this());
}

/**
 * Analytical age of the organ at a given length
 *
 * @param length   length of the organ [cm]
 */
double Organ::calcAge(double length)
{
	assert(length>=0 && "Organ::calcAge() negative Organ length");
	return getF_gf()->getAge(length,getParameter("r"),getParameter("k"),shared_from_this());
}



/**
 *  Creates nodes and node emergence times for a length l
 *
 *  Checks that each new segments length is <= dx but >= parent->minDx
 *
 *  @param l        total length of the segments that are created [cm]
 *  @param dt       time step [day]
 *  @param verbose  turns console output on or off
 *  @param PhytoIdx index of the growing phytomere (if nodal growth for stem)
 */
void Organ::createSegments(double l, double dt, bool verbose, int PhytoIdx)
{
    if (l==0) {
        std::cout << "Organ::createSegments: zero length encountered \n";
        return;
    }
    if (l<0) {
        std::cout << "Organ::createSegments: negative length encountered \n";
    }

    // shift first node to axial resolution
    double shiftl = 0; // length produced by shift
    int nn = nodes.size();
  if( PhytoIdx >= 0){ //if we are doing internodal growth,  PhytoIdx >= 0.
		auto o = children.at(PhytoIdx);
		nn = o->parentNI +1; //shift the last node of the phytomere n° PhytoIdx instead of the last node of the organ
	}	
	if (firstCall||(PhytoIdx >= 0)) { // first call of createSegments (in ::simulate)
        firstCall = false;
		bool notChildBaseNode = (children.empty() || (nn-1 != std::static_pointer_cast<Organ>(children.back())->parentNI));
		if ((nn>1) && (notChildBaseNode || (organType()!=Organism::ot_root)) ) { // don't move a child base node for roots
            Vector3d n2 = nodes[nn-2];
            Vector3d n1 = nodes[nn-1];
			Vector3d h;
			if(organType()==Organism::ot_root){
				h = n1.minus(n2);
			}else{h = n1;}//relative length for stem and leaves
            double olddx = h.length(); // length of last segment
			if (olddx<dx()*0.99) { // shift node instead of creating a new node
                shiftl = std::min(dx()-olddx, l);
                double sdx = olddx + shiftl; // length of new segment
                // Vector3d newdxv = getIncrement(n2, sdx);
                h.normalize();
				if(organType()==Organism::ot_root){
					nodes[nn-1] = Vector3d(n2.plus(h.times(sdx))); // n2.plus(newdxv)
                }else{nodes[nn-1] = h.times(sdx);}
				double et = this->calcCreationTime(getLength(true)+shiftl, dt);
                nodeCTs[nn-1] = et; // in case of impeded growth the node emergence time is not exact anymore, but might break down to temporal resolution
                moved = true;
                l -= shiftl;
				if (l<=0) { // ==0 should be enough
                    return;
                }
            } else {
                moved = false;
            }
        } else {
            moved = false;
        }
    }
    // create n+1 new nodes
    double sl = 0; // summed length of created segment
    int n = floor(l/dx());
	for (int i = 0; i < n + 1; i++) {

        double sdx; // segment length (<=dx)
        if (i<n) {  // normal case
            sdx = dx();
        } else { // last segment
            sdx = l-n*dx();
            if (sdx<dxMin()*0.99) { //plant.lock()->getMinDx()) { // quit if l is too small
                if (verbose&& sdx != 0) {
					std::cout <<"Organ::createSegments(): length increment below dxMin threshold ("<< sdx <<" < "<< dxMin() << ") and kept in memory\n";
                }
				if( PhytoIdx >= 0){
					this->epsilonDx += sdx;
				}else{this->epsilonDx = sdx;}
                return;
            }
			this->epsilonDx = 0; //no residual
        }
        sl += sdx;
		Vector3d newnode;
		if(organType()!=Organism::ot_root){
			newnode = Vector3d(sdx, 0., 0.);
		}else{ 
			Vector3d newdx = getIncrement(nodes.back(), sdx);
			newnode = Vector3d(nodes.back().plus(newdx));}
        double et = this->calcCreationTime(getLength(true)+shiftl+sl, dt);//here length or get length? it s the same because epsilonDx was set back to 0 at beginning of simulate no?
        // in case of impeded growth the node emergence time is not exact anymore,
        // but might break down to temporal resolution
        bool shift = (PhytoIdx >= 0); //node will be insterted between 2 nodes. only happens if we have internodal growth (PhytoIdx >= 0)
		addNode(newnode, et, size_t(nn+i), shift);
    }
}

/**
 * computes absolute coordinates from relative coordinates
 * when this function is called, the parent organ has already
 * its absolute coordinates
 * called by @see Plant::rel2abs
 *  @param dt      time step [day]
 */
void Organ::rel2abs(double dt) 
{
	double ageSwitch = getF_tf()->ageSwitch; //rename
	bool Leaf_tropismChange = (((age-dt)<= ageSwitch)&&(age >= ageSwitch)&&(ageSwitch >0))&&(organType()==Organism::ot_leaf);//re-compute tropism for leaf?
	bool Stem_tropismChange = (active&&(organType()==Organism::ot_stem));//re-compute tropism for stem?
	nodes[0] = getOrigin();//get absolute coordinates of first node via coordinates of parent
	for(size_t i=1; i<nodes.size(); i++){
		Vector3d newdx = nodes[i];
		//if new node or has an age-dependent tropism + reached age at which tropism changes. Might need to update the conditions if do new tropism functions
		//i.e., gradual change according to age
		if((i>= oldNumberOfNodes )||Leaf_tropismChange||Stem_tropismChange){
			double sdx = nodes[i].length();
			newdx = getIncrement(nodes[i-1], sdx, i-1);
		}
		nodes[i] = nodes[i-1].plus(newdx);
		
	}
	for(size_t i=0; i<children.size(); i++){
		(children[i])->rel2abs(dt);
	}//if carries children, update their coordinates from relative to absolute
	
	
}
/**
 * computes relative coordinates from absolute coordinates
 * when this function is called, the parent organ has already
 * its relative coordinates
 * called by @see Plant::abs2rel
 */
void Organ::abs2rel()
{
	for (int j = nodes.size(); j>1; j--) {
		nodes[j-1] = nodes.at(j-1).minus(nodes.at(j-2));
		}
	nodes[0] = Vector3d(0.,0.,0.);
	for(size_t i=0; i<children.size(); i++){
		(children[i])->abs2rel();
	}//if carry children, update their pos
	
}



/**
 * Returns the increment of the next segments
 *
 *  @param p       coordinates of previous node (in absolute coordinates)
 *  @param sdx     length of next segment [cm]
 *  @param n       index at which new node is to be inserted
 *  @return        the vector representing the increment
 */
Vector3d Organ::getIncrement(const Vector3d& p, double sdx, int n)
{
	Vector3d h = heading(n);
	Matrix3d ons = Matrix3d::ons(h);
	//use dx() rather rhan sdx to compute heading
	//to make tropism independante from growth rate
	int n_ = -2;
	if(organType() == Organism::ot_stem ){n_ = n;}
	Vector2d ab = getF_tf()->getHeading(p, ons, dx(),shared_from_this(), n_+1);
	Vector3d sv = ons.times(Vector3d::rotAB(ab.x,ab.y));
	return sv.times(sdx);
}

}
