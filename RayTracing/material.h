#ifndef MATERIAL_H
#define MATERIAL_H

#include "common.h"
#include "hittable.h"


class material {
public:
	virtual bool scatter(const ray& r, const HitRecord& rec, color& attenuation, ray& scattered) const = 0;
};


class lambertian : public material {
public:
	lambertian(const color& inAlbedo) :
		mAlbedo(inAlbedo)
	{}

	virtual bool scatter(const ray& inRay, const HitRecord& inRecord, color& inAttenuation, ray& inScattered) const override
	{
		auto scatter_direction = inRecord.normal + vec3::GetRandomUnitVector();

		// If the random unit vector we generate is exactly opposite the normal vector, 
		//the two will sum to zero, which will result in a zero scatter direction vector. 
		//This leads to bad scenarios later on (infinities and NaNs), 
		//so we need to intercept the condition before we pass it on.
		if (scatter_direction.IsNearZero())
			scatter_direction = inRecord.normal;

		inScattered = ray(inRecord.p, scatter_direction);
		inAttenuation = mAlbedo;
		return true;
	}

public:
	color mAlbedo;
};


class metal : public material {
public:
	metal(const color& inAlbedo, double inFuzziness = 0.0f) :
		mAlbedo(inAlbedo),
		mFuzziness(inFuzziness < 1 ? inFuzziness : 1)
	{}

	virtual bool scatter(const ray& inRay, const HitRecord& inRecord, color& inAttenuation, ray& inScattered) const override
	{
		vec3 reflected = Reflect(unit_vector(inRay.GetDirection()), inRecord.normal);
		inScattered = ray(inRecord.p, reflected + mFuzziness * vec3::GetRandomUnitVector());
		inAttenuation = mAlbedo;
		return (dot(inScattered.GetDirection(), inRecord.normal) > 0);
	}


public:
	color mAlbedo;
	double mFuzziness;
};


class dielectric : public material {
public:
	dielectric(float inRefractiveIndex, const color& inAlbedo = vec3(1.0, 1.0, 1.0)) :
		mRefractiveIndex(inRefractiveIndex),
		mAlbedo(inAlbedo)//attenuation is always 1 — the glass surface absorbs nothing.
	{}

	/*bool Refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted)
	{
		vec3 unit_v = unit_vector(v);
		float discriminat = 1.0 - ni_over_nt * ni_over_nt * (1 - dot(unit_v, n) * dot(unit_v, n));
	
		if (discriminat > 0)
		{
			refracted = ni_over_nt * (unit_v - n * dot(unit_v, n)) - n * sqrt(discriminat);
			return true;
		}

		//when discriminat < 0, we will encounter total reflection and have no refraction ray 
		//ray will be reflected back into the object.

		//when discriminat = 0, refracted ray is perpendicular to the surface normal - no reflection nor refraction.
		return false;
	}*/

	


	virtual bool scatter(const ray& inRay, const HitRecord& inRecord, color& inAttenuation, ray& inScattered) const override
	{
		inAttenuation = mAlbedo;
		double refraction_ratio = inRecord.front_face ? (1.0 / mRefractiveIndex) : mRefractiveIndex;
		vec3 unit_direction = unit_vector(inRay.GetDirection());

		////////////////////////////////////////////////////////////////////
		double cos_theta = fmin(dot(-unit_direction, inRecord.normal), 1.0);
		double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
		vec3 direction;

		if ((refraction_ratio * sin_theta > 1.0) || (Schlick(cos_theta, refraction_ratio) > gRandomDouble()))
		{
			// cannot refract, must reflect
			direction = Reflect(unit_direction, inRecord.normal);
		}
		else
		{
			// can Refract
			direction = Refract(unit_direction, inRecord.normal, refraction_ratio);
		}
		////////////////////////////////////////////////////////////////////

		//vec3 refracted = Refract(unit_direction, inRecord.normal, refraction_ratio);

		inScattered = ray(inRecord.p, direction);
		return true;
	}

public:
	float mRefractiveIndex;
	vec3 mAlbedo;

private:
	static double Schlick(double inCosine, double inRefractiveIndex)
	{
		float n_air = 1.000293;
		float r0 = (n_air - inRefractiveIndex) / (n_air + inRefractiveIndex);
		r0 *= r0;

		return  r0 + (1 - r0) * pow((1 - inCosine), 5);
	}
};
#endif
