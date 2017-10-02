// Quelle: https://github.com/erich666/jgt-code/blob/master/Volume_08/Number_1/Guigue2003/tri_tri_intersect.c

/*
*
*  Triangle-Triangle Overlap Test Routines
*  July, 2002
*  Updated December 2003
*
*  This file contains C implementation of algorithms for
*  performing two and three-dimensional triangle-triangle intersection test
*  The algorithms and underlying theory are described in
*
* "Fast and Robust Triangle-Triangle Overlap Test
*  Using Orientation Predicates"  P. Guigue - O. Devillers
*
*  Journal of Graphics Tools, 8(1), 2003
*
*  Several geometric predicates are defined.  Their parameters are all
*  points.  Each point is an array of two or three double precision
*  floating point numbers. The geometric predicates implemented in
*  this file are:
*
*    int tri_tri_overlap_test_3d(p1,q1,r1,p2,q2,r2)
*    int tri_tri_overlap_test_2d(p1,q1,r1,p2,q2,r2)
*
*    int tri_tri_intersection_test_3d(p1,q1,r1,p2,q2,r2,
*                                     coplanar,source,target)
*
*       is a version that computes the segment of intersection when
*       the triangles overlap (and are not coplanar)
*
*    each function returns 1 if the triangles (including their
*    boundary) intersect, otherwise 0
*
*
*  Other information are available from the Web page
*  http://www.acm.org/jgt/papers/GuigueDevillers03/
*
*/

/* function prototype */

// Three-dimensional Triangle-Triangle Overlap Test
bool tri_tri_overlap_test_3d(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2);


// Three-dimensional Triangle-Triangle Overlap Test
// additionaly computes the segment of intersection of the two triangles if it exists. 
// coplanar returns whether the triangles are coplanar, 
// source and target are the endpoints of the line segment of intersection 
bool tri_tri_intersection_test_3d(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, out bool coplanar, float3 source, float3 target);


bool coplanar_tri_tri3d(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, float3 normal_1, float3 normal_2);


// Two dimensional Triangle-Triangle Overlap Test
bool tri_tri_overlap_test_2d(float2 p1, float2 q1, float2 r1, float2 p2, float2 q2, float2 r2);


bool CHECK_MIN_MAX(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, inout float3 v1, inout float3 v2, inout float3 N1) {
	v1 = p2 - q1;
	v2 = p1 - q1;
	N1 = cross(v1, v2);
	v1 = q2 - q1;
	if (dot(v1, N1) > 0.0f) 
		return false;
	v1 = p2 - p1;
	v2 = r1 - p1;
	N1 = cross(v1, v2);
	v1 = r2 - p1;
	if (dot(v1, N1) > 0.0f) 
		return false;
	else 
		return true;
}



/* Permutation in a canonical form of T2's vertices */

bool TRI_TRI_3D(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, float dp2, float dq2, float dr2, inout float3 v1, inout float3 v2, inout float3 N1, inout float3 N2)
{
	if (dp2 > 0.0f)
	{
		if (dq2 > 0.0f)
            return CHECK_MIN_MAX(p1, r1, q1, r2, p2, q2, v1, v2, N1);
		else if (dr2 > 0.0f)
            return CHECK_MIN_MAX(p1, r1, q1, q2, r2, p2, v1, v2, N1);
		else
            return CHECK_MIN_MAX(p1, q1, r1, p2, q2, r2, v1, v2, N1);
    }
	else if (dp2 < 0.0f)
	{
		if (dq2 < 0.0f)
            return CHECK_MIN_MAX(p1, q1, r1, r2, p2, q2, v1, v2, N1);
		else if (dr2 < 0.0f)
            return CHECK_MIN_MAX(p1, q1, r1, q2, r2, p2, v1, v2, N1);
		else
            return CHECK_MIN_MAX(p1, r1, q1, p2, q2, r2, v1, v2, N1);
    }
	else
	{
		if (dq2 < 0.0f)
		{
			if (dr2 >= 0.0f)
                return CHECK_MIN_MAX(p1, r1, q1, q2, r2, p2, v1, v2, N1);
			else
                return CHECK_MIN_MAX(p1, q1, r1, p2, q2, r2, v1, v2, N1);
        }
		else if (dq2 > 0.0f)
		{
			if (dr2 > 0.0f)
                return CHECK_MIN_MAX(p1, r1, q1, p2, q2, r2, v1, v2, N1);
			else
                return CHECK_MIN_MAX(p1, q1, r1, q2, r2, p2, v1, v2, N1);
        }
		else
		{
			if (dr2 > 0.0f)
                return CHECK_MIN_MAX(p1, q1, r1, r2, p2, q2, v1, v2, N1);
			else if (dr2 < 0.0f)
                return CHECK_MIN_MAX(p1, r1, q1, r2, p2, q2, v1, v2, N1);
			else
				return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
		}
	}
}



/*
*
*  Three-dimensional Triangle-Triangle Overlap Test
*
*/


bool tri_tri_overlap_test_3d(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2)
{
	float dp1, dq1, dr1, dp2, dq2, dr2;
	float3 v1, v2;
	float3 N1, N2;

	/* Compute distance signs  of p1, q1 and r1 to the plane of
	triangle(p2,q2,r2) */


	v1 = p2 - r2;
	v2 = q2 - r2;
	N2 = cross(v1, v2);

	v1 = p1 - r2;
	dp1 = dot(v1, N2);
	v1 = q1 - r2;
	dq1 = dot(v1, N2);
	v1 = r1 - r2;
	dr1 = dot(v1, N2);

	if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))
		return false;

	/* Compute distance signs  of p2, q2 and r2 to the plane of
	triangle(p1,q1,r1) */


	v1 = q1 - p1;
	v2 = r1 - p1;
	N1 = cross(v1, v2);

	v1 = p2 - r1;
	dp2 = dot(v1, N1);
	v1 = q2 - r1;
	dq2 = dot(v1, N1);
	v1 = r2 - r1;
	dr2 = dot(v1, N1);

	if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f))
		return false;

	/* Permutation in a canonical form of T1's vertices */


	if (dp1 > 0.0f)
	{
		if (dq1 > 0.0f)
            return TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2, v1, v2, N1, N2);
		else if (dr1 > 0.0f)
            return TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2, v1, v2, N1, N2);
		else
            return TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2, v1, v2, N1, N2);
    }
	else if (dp1 < 0.0f)
	{
		if (dq1 < 0.0f)
            return TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2, v1, v2, N1, N2);
		else if (dr1 < 0.0f)
            return TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2, v1, v2, N1, N2);
		else
            return TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2, v1, v2, N1, N2);
    }
	else
	{
		if (dq1 < 0.0f)
		{
			if (dr1 >= 0.0f)
                return TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2, v1, v2, N1, N2);
			else
                return TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2, v1, v2, N1, N2);
        }
		else if (dq1 > 0.0f)
		{
			if (dr1 > 0.0f)
                return TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2, v1, v2, N1, N2);
			else
                return TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2, v1, v2, N1, N2);
        }
		else
		{
			if (dr1 > 0.0f)
                return TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2, v1, v2, N1, N2);
			else if (dr1 < 0.0f)
                return TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2, v1, v2, N1, N2);
			else
				return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
		}
	}
};



bool coplanar_tri_tri3d(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, float3 normal_1, float3 normal_2)
{
	float2 P1, Q1, R1;
	float2 P2, Q2, R2;

	float n_x, n_y, n_z;

	n_x = ((normal_1[0] < 0) ? -normal_1[0] : normal_1[0]);
	n_y = ((normal_1[1] < 0) ? -normal_1[1] : normal_1[1]);
	n_z = ((normal_1[2] < 0) ? -normal_1[2] : normal_1[2]);


	/* Projection of the triangles in 3D onto 2D such that the area of
	the projection is maximized. */


	if ((n_x > n_z) && (n_x >= n_y))
	{
		// Project onto plane YZ

		P1[0] = q1[2]; P1[1] = q1[1];
		Q1[0] = p1[2]; Q1[1] = p1[1];
		R1[0] = r1[2]; R1[1] = r1[1];

		P2[0] = q2[2]; P2[1] = q2[1];
		Q2[0] = p2[2]; Q2[1] = p2[1];
		R2[0] = r2[2]; R2[1] = r2[1];

	}
	else if ((n_y > n_z) && (n_y >= n_x))
	{
		// Project onto plane XZ

		P1[0] = q1[0]; P1[1] = q1[2];
		Q1[0] = p1[0]; Q1[1] = p1[2];
		R1[0] = r1[0]; R1[1] = r1[2];

		P2[0] = q2[0]; P2[1] = q2[2];
		Q2[0] = p2[0]; Q2[1] = p2[2];
		R2[0] = r2[0]; R2[1] = r2[2];

	}
	else
	{
		// Project onto plane XY

		P1[0] = p1[0]; P1[1] = p1[1];
		Q1[0] = q1[0]; Q1[1] = q1[1];
		R1[0] = r1[0]; R1[1] = r1[1];

		P2[0] = p2[0]; P2[1] = p2[1];
		Q2[0] = q2[0]; Q2[1] = q2[1];
		R2[0] = r2[0]; R2[1] = r2[1];
	}

	return tri_tri_overlap_test_2d(P1, Q1, R1, P2, Q2, R2);
}



/*
*
*  Three-dimensional Triangle-Triangle Intersection
*
*/

/*
This macro is called when the triangles surely intersect
It constructs the segment of intersection of the two triangles
if they are not coplanar.
*/

bool CONSTRUCT_INTERSECTION(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, inout float3 v1, inout float3 v2, inout float3 v, inout float3 N, inout float alpha, inout float3 source, inout float3 target, inout float3 N1, inout float3 N2)
{
	v1 = q1 - p1;
	v2 = r2 - p1;
	N = cross(v1, v2);
	v = p2 - p1;
	if (dot(v, N) > 0.0f)
	{
		v1 = r1 - p1;
		N = cross(v1, v2);
		if (dot(v, N) <= 0.0f)
		{
			v2 = q2 - p1;
			N = cross(v1, v2);
			if (dot(v, N) > 0.0f)
			{
				v1 = p1 - p2;
				v2 = p1 - r1;
				alpha = dot(v1, N2) / dot(v2, N2);
				v1 = alpha * v2;
				source = p1 - v1;
				v1 = p2 - p1;
				v2 = p2 - r2;
				alpha = dot(v1, N1) / dot(v2, N1);
				v1 = alpha * v2;
				target = p2 - v1;
				return true;
			}
			else
			{
				v1 = p2 - p1;
				v2 = p2 - q2;
				alpha = dot(v1, N1) / dot(v2, N1);
				v1 = alpha * v2;
				source = p2 - v1;
				v1 = p2 - p1;
				v2 = p2 - r2;
				alpha = dot(v1, N1) / dot(v2, N1);
				v1 = alpha * v2;
				target = p2 - v1;
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		v2 = q2 - p1;
		N = cross(v1, v2);
		if (dot(v, N) < 0.0f)
		{
			return false;
		}
		else
		{
			v1 = r1 - p1;
			N = cross(v1, v2);
			if (dot(v, N) >= 0.0f)
			{
				v1 = p1 - p2;
				v2 = p1 - r1;
				alpha = dot(v1, N2) / dot(v2, N2);
				v1 = alpha * v2;
				source = p1 - v1;
				v1 = p1 - p2;
				v2 = p1 - q1;
				alpha = dot(v1, N2) / dot(v2, N2);
				v1 = alpha * v2;
				target = p1 - v1;
				return true;
			}
			else
			{
				v1 = p2 - p1;
				v2 = p2 - q2;
				alpha = dot(v1, N1) / dot(v2, N1);
				v1 = alpha * v2;
				source = p2 - v1;
				v1 = p1 - p2;
				v2 = p1 - q1;
				alpha = dot(v1, N2) / dot(v2, N2);
				v1 = alpha * v2;
				target = p1 - v1;
				return true;
			}
		}
	}
}



bool TRI_TRI_INTER_3D(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, float dp2, float dq2, float dr2, inout float3 v1, inout float3 v2, inout float3 v, inout float3 N, inout float alpha, inout float3 source, inout float3 target, inout float3 N1, inout float3 N2, inout bool coplanar)
{
    if (dp2 > 0.0f)
    {
        if (dq2 > 0.0f) 
            return CONSTRUCT_INTERSECTION(p1, r1, q1, r2, p2, q2, v1, v2, v, N, alpha, source, target, N1, N2);
        else if (dr2 > 0.0f) 
            return CONSTRUCT_INTERSECTION(p1, r1, q1, q2, r2, p2, v1, v2, v, N, alpha, source, target, N1, N2);
        else
            return CONSTRUCT_INTERSECTION(p1, q1, r1, p2, q2, r2, v1, v2, v, N, alpha, source, target, N1, N2);
    }
    else if (dp2 < 0.0f)
    {
        if (dq2 < 0.0f) 
            return CONSTRUCT_INTERSECTION(p1, q1, r1, r2, p2, q2, v1, v2, v, N, alpha, source, target, N1, N2);
        else if (dr2 < 0.0f) 
            return CONSTRUCT_INTERSECTION(p1, q1, r1, q2, r2, p2, v1, v2, v, N, alpha, source, target, N1, N2);
        else
            return CONSTRUCT_INTERSECTION(p1, r1, q1, p2, q2, r2, v1, v2, v, N, alpha, source, target, N1, N2);
    }
    else
    {
        if (dq2 < 0.0f)
        {
            if (dr2 >= 0.0f)  
                return CONSTRUCT_INTERSECTION(p1, r1, q1, q2, r2, p2, v1, v2, v, N, alpha, source, target, N1, N2);
            else
                return CONSTRUCT_INTERSECTION(p1, q1, r1, p2, q2, r2, v1, v2, v, N, alpha, source, target, N1, N2);
        }
        else if (dq2 > 0.0f)
        {
            if (dr2 > 0.0f)
                return CONSTRUCT_INTERSECTION(p1, r1, q1, p2, q2, r2, v1, v2, v, N, alpha, source, target, N1, N2);
            else
                return CONSTRUCT_INTERSECTION(p1, q1, r1, q2, r2, p2, v1, v2, v, N, alpha, source, target, N1, N2);
        }
        else
        {
            if (dr2 > 0.0f) 
                return CONSTRUCT_INTERSECTION(p1, q1, r1, r2, p2, q2, v1, v2, v, N, alpha, source, target, N1, N2);
            else if (dr2 < 0.0f) 
                return CONSTRUCT_INTERSECTION(p1, r1, q1, r2, p2, q2, v1, v2, v, N, alpha, source, target, N1, N2);
            else
            {
                coplanar = 1;
                return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
            }
        }
    }
}


/*
The following version computes the segment of intersection of the
two triangles if it exists.
coplanar returns whether the triangles are coplanar
source and target are the endpoints of the line segment of intersection
*/

bool tri_tri_intersection_test_3d(float3 p1, float3 q1, float3 r1, float3 p2, float3 q2, float3 r2, out bool coplanar, float3 source, float3 target)
{
    float dp1, dq1, dr1, dp2, dq2, dr2;
    float3 v1, v2, v;
    float3 N1, N2, N;
    float alpha;

    coplanar = false;

	// Compute distance signs  of p1, q1 and r1 
	// to the plane of triangle(p2,q2,r2)


    v1 = p2 - r2;
    v2 = q2 - r2;
    N2 = cross(v1, v2);

    v1 = p1 - r2;
    dp1 = dot(v1, N2);
    v1 = q1 - r2;
    dq1 = dot(v1, N2);
    v1 = r1 - r2;
    dr1 = dot(v1, N2);

    if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))
        return false;

	// Compute distance signs  of p2, q2 and r2 
	// to the plane of triangle(p1,q1,r1)


    v1 = q1 - p1;
    v2 = r1 - p1;
    N1 = cross(v1, v2);

    v1 = p2 - r1;
    dp2 = dot(v1, N1);
    v1 = q2 - r1;
    dq2 = dot(v1, N1);
    v1 = r2 - r1;
    dr2 = dot(v1, N1);

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) 
        return false;

	// Permutation in a canonical form of T1's vertices


    if (dp1 > 0.0f)
    {
        if (dq1 > 0.0f) 
            return TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
        else if (dr1 > 0.0f) 
            return TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
        else
            return TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
    }
    else if (dp1 < 0.0f)
    {
        if (dq1 < 0.0f) 
            return TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
        else if (dr1 < 0.0f) 
            return TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
        else
            return TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
    }
    else
    {
        if (dq1 < 0.0f)
        {
            if (dr1 >= 0.0f) 
                return TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
            else
                return TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
        }
        else if (dq1 > 0.0f)
        {
            if (dr1 > 0.0f)
                return TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
            else
                return TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
        }
        else
        {
            if (dr1 > 0.0f) 
                return TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
            else if (dr1 < 0.0f) 
                return TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2, v1, v2, v, N, alpha, source, target, N1, N2, coplanar);
            else
            {
				// triangles are co-planar

                coplanar = true;
                return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
            }
        }
    }
}





/*
*
*  Two dimensional Triangle-Triangle Overlap Test
*
*/


/* some 2D macros */

float ORIENT_2D(float2 a, float2 b, float2 c)
{
	return (a[0] - c[0])*(b[1] - c[1]) - (a[1] - c[1])*(b[0] - c[0]);
}


bool INTERSECTION_TEST_VERTEX(float2 P1, float2 Q1, float2 R1, float2 P2, float2 Q2, float2 R2)
{
    if (ORIENT_2D(R2, P2, Q1) >= 0.0f)
    {
        if (ORIENT_2D(R2, Q2, Q1) <= 0.0f)
        {
            if (ORIENT_2D(P1, P2, Q1) > 0.0f)
            {
                if (ORIENT_2D(P1, Q2, Q1) <= 0.0f)
                    return true;
                else
                    return false;
            }
            else
            {
                if (ORIENT_2D(P1, P2, R1) >= 0.0f)
                {
                    if (ORIENT_2D(Q1, R1, P2) >= 0.0f)
                        return true;
                    else
                        return false;
                }
                else
                    return false;
            }
        }
        else if (ORIENT_2D(P1, Q2, Q1) <= 0.0f)
        {
            if (ORIENT_2D(R2, Q2, R1) <= 0.0f)
            {
                if (ORIENT_2D(Q1, R1, Q2) >= 0.0f)
                    return true;
                else
                    return false;
            }
            else
                return false;
        }
        else
            return false;
    }
    else if (ORIENT_2D(R2, P2, R1) >= 0.0f)
    {
        if (ORIENT_2D(Q1, R1, R2) >= 0.0f)
        {
            if (ORIENT_2D(P1, P2, R1) >= 0.0f)
                return true;
            else
                return false;
        }
        else if (ORIENT_2D(Q1, R1, Q2) >= 0.0f)
        {
            if (ORIENT_2D(R2, R1, Q2) >= 0.0f)
                return true;
            else
                return false;
        }
        else
            return false;
    }
    else
        return false;
}



bool INTERSECTION_TEST_EDGE(float2 P1, float2 Q1, float2 R1, float2 P2, float2 Q2, float2 R2)
{
    if (ORIENT_2D(R2, P2, Q1) >= 0.0f)
    {
        if (ORIENT_2D(P1, P2, Q1) >= 0.0f)
        {
            if (ORIENT_2D(P1, Q1, R2) >= 0.0f) 
                return true;
            else
                return false;
        }
        else
        {
            if (ORIENT_2D(Q1, R1, P2) >= 0.0f)
            {
                if (ORIENT_2D(R1, P1, P2) >= 0.0f) 
                    return true;
                else
                    return false;
            }
            else
                return false;
        }
    }
    else
    {
        if (ORIENT_2D(R2, P2, R1) >= 0.0f)
        {
            if (ORIENT_2D(P1, P2, R1) >= 0.0f)
            {
                if (ORIENT_2D(P1, R1, R2) >= 0.0f)
                    return true;
                else
                {
                    if (ORIENT_2D(Q1, R1, R2) >= 0.0f)
                        return true;
                    else
                        return false;
                }
            }
            else
                return false;
        }
        else
            return false;
    }
}



bool ccw_tri_tri_intersection_2d(float2 p1, float2 q1, float2 r1, float2 p2, float2 q2, float2 r2)
{
	if (ORIENT_2D(p2, q2, p1) >= 0.0f)
    {
		if (ORIENT_2D(q2, r2, p1) >= 0.0f)
        {
			if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                return true;
			else
                return INTERSECTION_TEST_EDGE(p1, q1, r1, p2, q2, r2);
        }
		else
        {
			if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                return INTERSECTION_TEST_EDGE(p1, q1, r1, r2, p2, q2);
			else 
                return INTERSECTION_TEST_VERTEX(p1, q1, r1, p2, q2, r2);
        }
	}
	else
    {
		if (ORIENT_2D(q2, r2, p1) >= 0.0f)
        {
			if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                return INTERSECTION_TEST_EDGE(p1, q1, r1, q2, r2, p2);
			else  
                return INTERSECTION_TEST_VERTEX(p1, q1, r1, q2, r2, p2);
        }
		else 
            return INTERSECTION_TEST_VERTEX(p1, q1, r1, r2, p2, q2);
    }
}


bool tri_tri_overlap_test_2d(float2 p1, float2 q1, float2 r1, float2 p2, float2 q2, float2 r2)
{
    if (ORIENT_2D(p1, q1, r1) < 0.0f)
    {
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, q2, r2);
    }
    else
    {
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, q2, r2);
    }
}
