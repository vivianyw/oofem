/*
 *
 *                 #####    #####   ######  ######  ###   ###
 *               ##   ##  ##   ##  ##      ##      ## ### ##
 *              ##   ##  ##   ##  ####    ####    ##  #  ##
 *             ##   ##  ##   ##  ##      ##      ##     ##
 *            ##   ##  ##   ##  ##      ##      ##     ##
 *            #####    #####   ##      ######  ##     ##
 *
 *
 *             OOFEM : Object Oriented Finite Element Code
 *
 *               Copyright (C) 1993 - 2013   Borek Patzak
 *
 *
 *
 *       Czech Technical University, Faculty of Civil Engineering,
 *   Department of Structural Mechanics, 166 29 Prague, Czech Republic
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef elementgeometry_h
#define elementgeometry_h

#include "femcmpnn.h"
#include "floatmatrix.h"
#include "floatarray.h"

#include "alist.h"
#include "intarray.h"
#include "error.h"
#include "integrationrule.h"
#include "chartype.h"
#include "elementgeometrytype.h"
#include "equationid.h"
#include "valuemodetype.h"
#include "internalstatetype.h"
#include "internalstatevaluetype.h"
#include "elementextension.h"
#include "entityrenumberingscheme.h"
#include "unknowntype.h"
#include "unknownnumberingscheme.h"

#ifdef __OOFEG
 #include "node.h"
 #include "engngm.h"
#endif

#include <cstdio>

///@name Input fields for general element.
//@{
#define _IFT_ElementGeometry_mat "mat"
#define _IFT_ElementGeometry_crosssect "crosssect"
#define _IFT_ElementGeometry_nodes "nodes"

#define _IFT_ElementGeometry_lcs "lcs"
#define _IFT_ElementGeometry_nip "nip"
#define _IFT_ElementGeometry_activityTimeFunction "activityltf"
//@}

namespace oofem {
class TimeStep;
class Node;
class Material;
class GaussPoint;
class FloatMatrix;
class IntArray;
class CrossSection;
class ElementSide;
class FEInterpolation;
class Load;
class BoundaryLoad;

#ifdef __PARALLEL_MODE
class CommunicationBuffer;

/**
 * In parallel mode, this type indicates the mode of element.
 * In the case of element cut mode, the cut element is local on all partitions sharing it.
 * Some of such element nodes are local and some are remote. The local nodes are completely
 * surrounded by local element on particular partition.
 */
enum elementParallelMode {
    Element_local, ///< Element is local, there are no contributions from other domains to this element.
    // Element_shared, ///< Element is shared by neighboring partitions - not implemented.
    Element_remote, ///< Element in active domain is only mirror of some remote element.
};

#endif


/**
 * Abstract base class for all finite element geometries. 
 * This abstract class declares (and possibly implements) general data and methods
 * common to all element types. General methods for obtaining characteristic vectors,
 * matrices and values are introduced and should be used instead of calling directly
 * specific member functions (these must be overloaded by derived analysis-specific
 * classes in order to invoke proper method according to type of component requested).
 *
 * There are some general rules, that programmer must take into account.
 * - ElementGeometry stores the numbers of its dofmanagers in dofManArray.
 *   These include nodes, element sides and internal DOFs that are not condensed at element level.
 *   Their order and meaning are determined by element definition.
 *   Local ordering of dofs for particular element is determined by local numbering of
 *   dofmanagers and their corresponding dofs. DOFS necessary for particular node/side is specified using node/side dof mask.
 *   Local DOF ordering must be taken into account when assembling various local characteristic
 *   vectors and matrices.
 */
class OOFEM_EXPORT ElementGeometry : public FEMComponent
{
protected:
    /// Number of dofmanagers
    int numberOfDofMans;
    /// Array containing dofmanager numbers.
    IntArray dofManArray;
    /// Number of associated material.
    int material;
    /// Number of associated cross section.
    int crossSection;
	/// Element activity time function. If defined, nonzero value indicates active receiver, zero value inactive element.
	int activityTimeFunction;

    /// Number of integration rules used by receiver.
    int numberOfIntegrationRules;
    /**
     * List of integration rules of receiver (each integration rule contains associated
     * integration points also). This list should contain only such integration rules,
     * that are used to integrate results depending on load time history. For all integration
     * points in these rules, history variables are stored and updated.
     * For integrations, where history stored in gauss points is not necessary
     * (mass matrix integration) and different integration rule is needed, one should preferably
     * use temporarily created integration rule.
     */
    IntegrationRule **integrationRulesArray;

    /// Transformation material matrix, used in orthotropic and anisotropic materials, global->local transformation
    FloatMatrix elemLocalCS;
    /**
     * In parallel mode, globalNumber contains globally unique DoFManager number.
     * The component number, inherited from FEMComponent class contains
     * local domain number.
     */
    int globalNumber;

    /**
     * Number of integration points as specified by nip.
     */
    int numberOfGaussPoints;

#ifdef __PARALLEL_MODE
    elementParallelMode parallel_mode;
    /**
     * List of partition sharing the shared element or
     * remote partition containing remote element counterpart.
     */
    IntArray partitions;
#endif

public:
    /**
     * Constructor. Creates an element with number n belonging to domain aDomain.
     * @param n Element's number
     * @param aDomain Pointer to the domain to which element belongs.
     */
    ElementGeometry(int n, Domain *aDomain);
    /// Virtual destructor.
    virtual ~ElementGeometry();

  
   /**
     * Returns dofmanager dof mask for node. This mask defines the dofs which are used by element
     * in node. Mask influences the code number ordering for particular node. Code numbers are
     * ordered according to node order and dofs belonging to particular node are ordered
     * according to this mask. If element requests dofs using node mask which are not in node
     * then error is generated. This masking allows node to be shared by different elements with
     * different dofs in same node. Elements local code numbers are extracted from node using
     * this mask. Must be defined by particular element.
     *
     * @param inode  Mask is computed for local dofmanager with inode number.
     * @param ut     Equation DOFs belong to.
     * @param answer Mask for node.
     */
    virtual void giveDofManDofIDMask(int inode, EquationID ut, IntArray &answer) const { answer.resize(0); }
    /**
     * Calls giveDofManDofIDMask with the default equation id for the type of problem.
     * @todo Cant have a pure virtual method because of the hacks in HellmichMaterial :: createMaterialGp()
     */
    virtual void giveDefaultDofManDofIDMask(int inode, IntArray &answer) const { }
    /**
     * Returns internal  dofmanager dof mask for node. This mask defines the dofs which are used by element
     * in node. Mask influences the code number ordering for particular node. Code numbers are
     * ordered according to node order and dofs belonging to particular node are ordered
     * according to this mask. If element requests dofs using node mask which are not in node
     * then error is generated. This masking allows node to be shared by different elements with
     * different dofs in same node. Elements local code numbers are extracted from node using
     * this mask. Must be defined by particular element.
     *
     * @param inode Mask is computed for local dofmanager with inode number.
     * @param ut Unknown type (support for several independent numberings within problem)
     * @param answer mask for node.
     */
    virtual void giveInternalDofManDofIDMask(int inode, EquationID ut, IntArray &answer) const
    { answer.resize(0); }
    
	/**
	* Returns i-th internal element dof manager of the receiver
	* @param i Internal number of DOF.
	* @return DOF number i.
	*/
	virtual DofManager *giveInternalDofManager(int i) const {
	        OOFEM_ERROR("No such DOF available on Element %d", number);
		return NULL;
	}
	
	/**
	* @return Number of internal DOF managers of element.
	*/
	virtual int giveNumberOfInternalDofManagers() const { return 0; }
	
	/**
     * Calls giveInternalDofManDofIDMask with the default equation id for the type of problem.
     */	
	virtual void giveDefaultInternalDofManDofIDMask(int inode, IntArray &answer) const { answer.resize(0); }
    /**
     * Returns element dof mask for node. This mask defines the dof ordering of the element interpolation.
     * Must be defined by particular element.
     *
     * @param ut Equation DOFs belong to.
     * @param answer DOF mask for receiver.
     */
	
	
    virtual void giveElementDofIDMask(EquationID ut, IntArray &answer) const { answer.resize(0); }
    /**
     * Returns volume related to given integration point. Used typically in subroutines,
     * that perform integration over element volume. Should be implemented by particular
     * elements.
     * @see GaussPoint
     * @param gp Integration point for which volume is computed.
     * @return Volume for integration point.
     */
    virtual double computeVolumeAround(GaussPoint *gp) { return 0.; }
    /// Computes the volume, area or length of the element depending on its spatial dimension.
    double computeVolumeAreaOrLength();
    /**
     * Computes the size of the element defined as its length.
     * @return Length, square root of area or cube root of volume (depending on spatial dimension).
     */
    double computeMeanSize();
    /**
     * Computes the volume.
     * @return Volume of element.
     */
    virtual double computeVolume();
    /**
     * Computes the area (zero for all but 2d geometries).
     * @return Element area.
     */
    virtual double computeArea();
    /**
     * Computes the length (zero for all but 1D geometries)
     * @return Element length.
     */
    virtual double computeLength();
    // If the need arises;
    /*
     * Computes the length of an edge.
     * @param iedge Edge number.
     * @return Edge length.
     */
    //virtual double computeEdgeLength(int iedge) { return 0.0; }
    /*
     * Computes the area of a surface.
     * @param isurf Surface number.
     * @param Surface area.
     */
    //virtual double computeSurfaceArea(int isurf) { return 0.0; }

    // data management
    /**
     * Translates local to global indices for dof managers.
     * @param i Local index of dof manager.
     * @return Global number of i-th dofmanager of element
     */
    int giveDofManagerNumber(int i) const { return dofManArray.at(i); }
    /// @return Receivers list of dof managers.
    IntArray &giveDofManArray() { return dofManArray; }
    /**
     * @param i Local index of the dof manager in element.
     * @return The i-th dofmanager of element.
     */
    DofManager *giveDofManager(int i) const;
    /**
     * Returns reference to the i-th node of element.
     * Default implementation returns i-th dofmanager of element converted to
     * Node class (check is made).
     * @param i Local index of node in element.
     * @return Requested node.
     */
    virtual Node *giveNode(int i) const;
    /**
     * Returns reference to the i-th side  of element.
     * Default implementation returns i-th dofmanager of element converted to
     * ElementSide class (check is made).
     * @param i Side number.
     * @return Requested element side.
     */
    virtual ElementSide *giveSide(int i) const;
    /// @return Interpolation of the element geometry, or NULL if none exist.
    virtual FEInterpolation *giveInterpolation() const { return NULL; }
	/// @return i-th Interpolation of the element geometry, or NULL if none exist.
	virtual FEInterpolation *giveInterpolation(int i) const { return NULL; }
    /**
     * Returns the interpolation for the specific dof id.
     * Special elements which uses a mixed interpolation should reimplement this method.
     * @param id ID of the dof for the for the requested interpolation.
     * @return Appropriate interpolation, or NULL if none exists.
     */
	virtual FEInterpolation *giveInterpolation(DofIDItem id) const { return giveInterpolation(); }
    /// @return Reference to the associated material of element.
    Material *giveMaterial();
    /// @return Reference to the associated crossSection of element.
    CrossSection *giveCrossSection();
    /**
     * Sets the material of receiver.
     * @param matIndx Index of new material.
     */
    void setMaterial(int matIndx) { this->material = matIndx; }
    /**
     * Sets the cross section model of receiver.
     * @param csIndx Index of new cross section.
     */
    void setCrossSection(int csIndx) { this->crossSection = csIndx; }

    /// @return Number of dofmanagers of receiver.
    int giveNumberOfDofManagers() const { return numberOfDofMans; }
    /**
     * Returns number of nodes of receiver.
     * Default implementation returns number of dofmanagers of element
     * @return Number of nodes of element.
     */
    virtual int giveNumberOfNodes() const { return numberOfDofMans; }
    /**
     * Sets receiver dofManagers.
     * @param dmans Array with dof manager indices.
     */
    void setDofManagers(const IntArray &dmans);

    /**
     * Sets integration rules.
     * @param irlist List of integration rules.
     */
    void setIntegrationRules(AList< IntegrationRule > *irlist);
    /**
     * Returns integration domain for receiver, used to initialize
     * integration point over receiver volume.
     * Default behavior is taken from the default interpolation.
     * @return Integration domain of element.
     */
    virtual integrationDomain giveIntegrationDomain() const;
    /**
     * Returns material mode for receiver integration points. Should be specialized.
     * @return Material mode of element.
     */
    virtual MaterialMode giveMaterialMode() { return _Unknown; }
    /**
     * Assembles the code numbers of given integration element (sub-patch)
     * This is done by obtaining list of nonzero shape functions and
     * by collecting the code numbers of nodes corresponding to these
     * shape functions.
     * @return Nonzero if integration rule code numbers differ from element code numbers.
     */
    virtual int giveIntegrationRuleLocalCodeNumbers(IntArray &answer, IntegrationRule *ie, EquationID ut)
    { return 0; }

    // Returns number of sides (which have unknown dofs) of receiver
    //int giveNumberOfSides () {return numberOfSides;}

    /// @return Corresponding element region. Currently corresponds to cross section model number.
    int giveRegionNumber();

   
	/**
	* Returns the element  activity time function.
	* @returns Number, nonzero value indicates active receiver, zero value inactive element.
	*/
	int giveActivityTimeFunction() { return activityTimeFunction; }

    /**
     * Performs consistency check. This method is called at startup for all elements
     * in particular domain. This method is intended to check data compatibility.
     * Particular element types should test if compatible material
     * and crossSection both with required capabilities are specified.
     * Derived classes should provide their own analysis specific tests.
     * Some printed input if incompatibility is found should be provided
     * (error or warning member functions).
     * Method can be also used to initialize some variables, since
     * this is invoked after all domain components are instanciated.
     * @return Zero value if check fail, otherwise nonzero.
     */
    virtual int checkConsistency() { return 1; }

    /**
     * @return True, if receiver is activated for given solution step, otherwise false.
     */
    virtual bool isActivated(TimeStep *tStep);

    // time step initialization (required for some non-linear solvers)
    /**
     * Initializes receivers state to new time step. It can be used also if
     * current time step must be re-started. Default implementation
     * invokes initForNewStep member function for all defined integrationRules.
     * Thus all state variables in all defined integration points are re initialized.
     * @see IntegrationRule::initForNewStep.
     */
    virtual void initForNewStep();
    /**
     * Returns the element geometry type.
     * This information is assumed to be of general interest, but
     * it is required only for some specialized tasks.
     * @return Geometry type of element.
     */
    virtual Element_Geometry_Type giveGeometryType() const;
    
	/**
     * Returns the element spatial dimension (1, 2, or 3).
     * This is completely based on the geometrical shape, so a plane in space counts as 2 dimensions.
     * @return Number of spatial dimensions of element.
     */
    virtual int giveSpatialDimension();
    /**
     * @return Number of boundaries of element.
     */
    virtual int giveNumberOfBoundarySides();
    /**
     * Returns id of default integration rule. Various element types can use
     * different integration rules for implementation of selective or reduced
     * integration of selected components. One particular integration rule from
     * defined integration rules is default. There may be some operations (defined
     * by parent analysis type class) which use default integration rule.
     * @return Id of default integration rule. (index into integrationRulesArray).
     */
    virtual int giveDefaultIntegrationRule() const { return 0; }
    /**
     * Access method for default integration rule.
     * @return Pointer to default integration rule.
     * @see giveDefaultIntegrationRule
     */
    virtual IntegrationRule *giveDefaultIntegrationRulePtr() {
        if ( this->giveNumberOfIntegrationRules() == 0 ) {
            return NULL;
        } else {
            return this->integrationRulesArray [ giveDefaultIntegrationRule() ];
        }
    }

	/// Performs post initialization steps.
	virtual void postInitialize();


	/**
	* Updates element state after equilibrium in time step has been reached.
	* Default implementation updates all integration rules defined by
	* integrationRulesArray member variable. Doing this, all integration points
	* and their material statuses are updated also. All temporary history variables,
	* which now describe equilibrium state are copied into equilibrium ones.
	* The existing internal state is used for update.
	* @param tStep Time step for newly reached state.
	* @see Material::updateYourself
	* @see IntegrationRule::updateYourself
	* @see GaussPoint::updateYourself
	* @see Element::updateInternalState
	*/
	virtual void updateYourself(TimeStep *tStep);

    /// @return Number of integration rules for element.
    int giveNumberOfIntegrationRules() { return this->numberOfIntegrationRules; }
    /**
     * @param i Index of integration rule.
     * @return Requested integration rule.
     */
    virtual IntegrationRule *giveIntegrationRule(int i) { return integrationRulesArray [ i ]; }
   
	/**
	* Updates element state after equilibrium in time step has been reached.
	* Default implementation updates all integration rules defined by
	* integrationRulesArray member variable. Doing this, all integration points
	* and their material statuses are updated also. All temporary history variables,
	* which now describe equilibrium state are copied into equilibrium ones.
	* The existing internal state is used for update.
	* @param tStep Time step for newly reached state.
	* @see Material::updateYourself
	* @see IntegrationRule::updateYourself
	* @see GaussPoint::updateYourself
	* @see Element::updateInternalState
	*/
	virtual void updateInternalState(TimeStep *tStep) { }

    //@}

    /**@name Methods required by some specialized models */
    //@{
    /**
     * Returns the integration point corresponding value in full form.
     * @param answer Contain corresponding integration point value, zero sized if not available.
     * @param gp Integration point to check.
     * @param type Determines the type of internal variable.
     * @param tStep Time step.
     * @return Nonzero if o.k, zero otherwise.
     */
    virtual int giveIPValue(FloatArray &answer, GaussPoint *gp, InternalStateType type, TimeStep *tStep);

    // characteristic length in gp (for some material models)
    /**
     * Default implementation returns length of element projection into specified direction.
     * @return Element length in given direction.
     */
    virtual double giveLenghtInDir(const FloatArray &normalToCrackPlane);
    /**
     * Returns characteristic length of element in given integration point and in
     * given direction. Required by material models relying on crack-band approach to achieve
     * objectivity with respect to mesh size
     * @param gp Integration point.
     * @param normalToCrackPlane Normal to crack plane.
     * @return Characteristic length of element in given integration point and direction.
     */
    virtual double giveCharacteristicLenght(GaussPoint *gp, const FloatArray &normalToCrackPlane) { return 0.; }
    /**
     * Returns characteristic element size for a given integration point and
     * given direction. Required by material models relying on crack-band approach to achieve
     * objectivity with respect to mesh size.
     * Various techniques can be selected by changing the last parameter.
     * @param gp Integration point.
     * @param normalToCrackPlane Normal to assumed crack plane (some methods use it, some methods recompute it and return the new value).
     * @param method Selection of the specific method to be used.
     * @return Characteristic length of element in given integration point and direction.
     */
    virtual double giveCharacteristicSize(GaussPoint *gp, FloatArray &normalToCrackPlane, ElementCharSizeMethod method) { return giveCharacteristicLenght(gp, normalToCrackPlane); }
    /**
     * Returns the size (length, area or volume depending on element type) of the parent
     * element. E.g. 4.0 for a quadrilateral.
     */
    virtual double giveParentElSize() const { return 0.0; }
   
    /**
     * Computes the global coordinates from given element's local coordinates.
     * @param answer Requested global coordinates.
     * @param lcoords Local coordinates.
     * @return Nonzero if successful, zero otherwise.
     */
    virtual int computeGlobalCoordinates(FloatArray &answer, const FloatArray &lcoords);
    /**
     * Computes the element local coordinates from given global coordinates.
     * Should compute local coordinates even if point is outside element (for mapping purposes in adaptivity)
     * @param answer Local coordinates.
     * @param gcoords Global coordinates.
     * @return Nonzero if point is inside element; zero otherwise.
     */
    virtual bool computeLocalCoordinates(FloatArray &answer, const FloatArray &gcoords);
    /**
     * Returns local coordinate system of receiver
     * Required by material models with ortho- and anisotrophy.
     * Returns a unit vectors of local coordinate system at element stored row-wise.
     * If local system is equal to global one, set answer to empty matrix and return zero value.
     * @return nonzero if answer computed, zero value if answer is empty, i.e. no transformation is necessary.
     */
    virtual int giveLocalCoordinateSystem(FloatMatrix &answer);

    /**
     * Computes mid-plane normal of receiver at integration point.
     * Only for plane elements in space (3d)  (shells, plates, ....).
     * @param answer The mid plane normal.
     * @param gp The integration point at which to calculate the normal.
     */
    virtual void computeMidPlaneNormal(FloatArray &answer, const GaussPoint *gp);
    
	/**
	* Initializes the internal state variables stored in all IPs according to state in given domain.
	* Used in adaptive procedures.
	* @param oldd Old mesh reference.
	* @param tStep Time step.
	* @return Nonzero if o.k, otherwise zero.
	*/
	virtual int adaptiveMap(Domain *oldd, TimeStep *tStep);
	/**
	* Maps the internal state variables stored in all IPs from the old domain to the new domain.
	* @param iOldDom Old domain.
	* @param iTStep Time step.
	* @return Nonzero if o.k, otherwise zero.
	*/
	virtual int mapStateVariables(Domain &iOldDom, const TimeStep &iTStep);
	/**
	* Updates the internal state variables stored in all IPs according to
	* already mapped state.
	* @param tStep Time step.
	* @return Nonzero if o.k, otherwise zero.
	*/
	virtual int adaptiveUpdate(TimeStep *tStep) { return 1; }
	/**
	* Finishes the mapping for given time step.
	* @param tStep Time step.
	* @return Nonzero if o.k, otherwise zero.
	*/
	virtual int adaptiveFinish(TimeStep *tStep);


    /**
     * Local renumbering support. For some tasks (parallel load balancing, for example) it is necessary to
     * renumber the entities. The various FEM components (such as nodes or elements) typically contain
     * links to other entities in terms of their local numbers, etc. This service allows to update
     * these relations to reflect updated numbering. The renumbering function is passed, which is supposed
     * to return an updated number of specified entity type based on old number.
     * @param f Decides the renumbering.
     */
    virtual void updateLocalNumbering(EntityRenumberingFunctor &f);

    /// Integration point evaluator, loops over receiver IP's and calls given function (passed as f parameter) on them. The IP is parameter to function f.
    template< class T >void ipEvaluator( T * src, void ( T :: *f )( GaussPoint * gp ) );
    /// Integration point evaluator, loops over receiver IP's and calls given function (passed as f parameter) on them. The IP is parameter to function f as well as additional array.
    template< class T, class S >void ipEvaluator(T * src, void ( T :: *f )( GaussPoint *, S & ), S & _val);

    //@}


#ifdef __OOFEG
    //
    // Graphics output
    //
    virtual void drawYourself(oofegGraphicContext &context);
    virtual void drawAnnotation(oofegGraphicContext &mode);
    virtual void drawRawGeometry(oofegGraphicContext &mode) { }
    virtual void drawDeformedGeometry(oofegGraphicContext &mode, UnknownType) { }
    virtual void drawScalar(oofegGraphicContext &context) { }
    virtual void drawSpecial(oofegGraphicContext &context) { }
    // added in order to hide IP element details from oofeg
    // to determine the max and min local values, when recovery does not takes place
    virtual void giveLocalIntVarMaxMin(oofegGraphicContext &context, TimeStep *, double &emin, double &emax) { emin = emax = 0.0; }

    /**
     * Returns internal state variable (like stress,strain) at node of element in Reduced form,
     * the way how is obtained is dependent on InternalValueType.
     * The value may be local, or smoothed using some recovery technique.
     * Returns zero if element is unable to respond to request.
     * @param answer Contains result, zero sized if not supported.
     * @param type Determines the internal variable requested (physical meaning).
     * @param mode Determines the mode of variable (recovered, local, ...).
     * @param node Node number, for which variable is required.
     * @param tStep Time step.
     * @return Nonzero if o.k, zero otherwise.
     */
    virtual int giveInternalStateAtNode(FloatArray &answer, InternalStateType type, InternalStateMode mode,
                                        int node, TimeStep *tStep);
    /**
     * Returns internal state variable (like stress,strain) at side of element in Reduced form
     * If side is possessing DOFs, otherwise recover techniques will not work
     * due to absence of side-shape functions.
     * @param answer Contains result, zero sized if not supported.
     * @param type Determines the internal variable requested (physical meaning).
     * @param mode Determines the mode of variable (recovered, local, ...).
     * @param side Side number, for which variable is required.
     * @param tStep Time step.
     * @return Nonzero if o.k, zero otherwise.
     */
    virtual int giveInternalStateAtSide(FloatArray &answer, InternalStateType type, InternalStateMode mode,
                                        int side, TimeStep *tStep)
    {
        answer.resize(0);
        return 0;
    }

    /// Shows sparse structure
    virtual void showSparseMtrxStructure(CharType mtrx, oofegGraphicContext &gc, TimeStep *tStep) { }
    /// Shows extended sparse structure (for example, due to nonlocal interactions for tangent stiffness)
    virtual void showExtendedSparseMtrxStructure(CharType mtrx, oofegGraphicContext &gc, TimeStep *tStep) { }

#endif

    /**
     * @return Receivers globally unique number (label).
     */
    int giveLabel() const { return globalNumber; }
    /**
     * @return Receivers globally unique number.
     */
    int giveGlobalNumber() const { return globalNumber; }
    /**
     * Sets receiver globally unique number.
     * @param num New unique number.
     */
    void setGlobalNumber(int num) { globalNumber = num; }

#ifdef __PARALLEL_MODE
    /**
     * Return elementParallelMode of receiver. Defined for __Parallel_Mode only.
     */
    elementParallelMode giveParallelMode() const { return parallel_mode; }
    /// Sets parallel mode of element
    void setParallelMode(elementParallelMode _mode) { parallel_mode = _mode; }
    /**
     * Returns the parallel mode for particular knot span of the receiver.
     * The knot span identifies the sub-region of the finite element.
     */
    virtual elementParallelMode giveKnotSpanParallelMode(int) const { return parallel_mode; }

    /**
     * Pack all necessary data of element (according to its parallel_mode) integration points
     * into given communication buffer. The corresponding cross section service is invoked, which in
     * turn should invoke material model service for particular integration point. The
     * nature of packed data is material model dependent.
     * Typically, for material of "local" response (response depends only on integration point local state)
     * no data are exchanged. For "nonlocal" constitutive models the send/receive of local values which
     * undergo averaging is performed between local and corresponding remote elements.
     * @param buff communication buffer
     * @param tStep solution step.
     */
    int packUnknowns(CommunicationBuffer &buff, TimeStep *tStep);
    /**
     * Unpack and updates all necessary data of element (according to its parallel_mode) integration points
     * into given communication buffer.
     * @see packUnknowns service.
     * @param buff communication buffer
     * @param tStep solution step.
     */
    int unpackAndUpdateUnknowns(CommunicationBuffer &buff, TimeStep *tStep);
    /**
     * Estimates the necessary pack size to hold all packed data of receiver.
     * The corresponding cross section service is invoked, which in
     * turn should invoke material model service for particular integration point. The
     * nature of packed data is material model dependent.  */
    int estimatePackSize(CommunicationBuffer &buff);
    /**
     * Returns partition list of receiver.
     * @return partition array.
     */
    const IntArray *givePartitionList() const { return & partitions; }
    /**
     * Sets partition list of receiver
     */
    void setPartitionList(IntArray &pl) { partitions = pl; }
    /**
     * Returns the weight representing relative computational cost of receiver
     * The reference element is triangular plane stress element with
     * linear approximation, single integration point and linear isotropic material.
     * Its weight is equal to 1.0
     * Default implementation computes average computational cost of cross section model (this include material as well)
     * and multiplies it by element type weight (obtained by giveRelativeSelfComputationalCost())
     * The other elements should compare to this reference element.
     */
    virtual double predictRelativeComputationalCost();
    /**
     * Returns the weight representing relative computational cost of receiver
     * The reference element is triangular plane stress element.
     * Its weight is equal to 1.0
     * The other elements should compare to this reference element.
     */
    virtual double giveRelativeSelfComputationalCost() { return 1.0; }
    /**
     * Returns the relative redistribution cost of the receiver
     */
    virtual double predictRelativeRedistributionCost() { return 1.0; }
#endif

public:
    

    // Overloaded methods:
    virtual IRResultType initializeFrom(InputRecord *ir);
    virtual void giveInputRecord(DynamicInputRecord &input);
    virtual contextIOResultType saveContext(DataStream *stream, ContextMode mode, void *obj = NULL);
    virtual contextIOResultType restoreContext(DataStream *stream, ContextMode mode, void *obj = NULL);
    virtual void printOutputAt(FILE *file, TimeStep *tStep);
    virtual const char *giveClassName() const { return "ElementGeometry"; }

protected:
    /**
     * Initializes the array of integration rules and numberOfIntegrationRules member variable.
     * Element can have multiple integration rules for different tasks.
     * For example structural element family class uses this feature to implement
     * transparent support for reduced and selective integration of some strain components.
     * Must be defined by terminator classes.
     * @see IntegrationRule
     */
    virtual void computeGaussPoints() { }
};

template< class T >void
ElementGeometry :: ipEvaluator( T *src, void ( T :: *f )( GaussPoint * gp ) )
{
    int ir, ip, nip;
    GaussPoint *gp;

    for ( ir = 0; ir < numberOfIntegrationRules; ir++ ) {
        nip = integrationRulesArray [ ir ]->giveNumberOfIntegrationPoints();
        for ( ip = 0; ip < nip; ip++ ) {
            gp = integrationRulesArray [ ir ]->getIntegrationPoint(ip);
            ( src->*f )(gp);
        }
    }
}

template< class T, class S >void
ElementGeometry :: ipEvaluator(T *src, void ( T :: *f )( GaussPoint *, S & ), S &_val)
{
    int ir, ip, nip;
    GaussPoint *gp;

    for ( ir = 0; ir < numberOfIntegrationRules; ir++ ) {
        nip = integrationRulesArray [ ir ]->giveNumberOfIntegrationPoints();
        for ( ip = 0; ip < nip; ip++ ) {
            gp = integrationRulesArray [ ir ]->getIntegrationPoint(ip);
            ( src->*f )(gp, _val);
        }
    }
}
} // end namespace oofem
#endif //elementgeometry_h