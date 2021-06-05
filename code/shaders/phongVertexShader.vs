#version 330
	layout(location = 0) in vec3 in_Position;
	layout(location = 1)in vec3 in_Normal;
	layout(location = 2)in vec2 uvs;
	in vec4 geomPos;
	out vec2 outUvs;
	out vec3 outNormal;
	out vec3 lightIntensity;
	uniform mat4 objMat;
	uniform mat4 mv_Mat;
	uniform mat4 mvpMat;
	uniform vec4 lightPos;
	uniform float InShininess;
	
	
	vec3 ambientLight = vec3(1.0, 1.0, 1.0);
	vec3 diffuseLight = vec3(1.0, 1.0, 1.0);
	vec3 specularLight = vec3(1.0, 1.0, 1.0);
	float shininess = 4096;

	void main() {
		outUvs = uvs;
		outNormal = in_Normal;
		shininess = InShininess;
		
		// Ambient Light
		vec4 n = normalize(objMat * vec4(in_Normal, 1.0));
		vec4 camCoords = mv_Mat * vec4(in_Position, 1.0);
		vec3 ambient = ambientLight;
		
		// Diffuse Light
		vec4 s = normalize(vec4(lightPos.xyz - camCoords.xyz, 1.0));
		float sDotN = max(dot(s, n), 0.0);
		if(sDotN < 0.2)
			sDotN = 0;
		if(sDotN >= 0.2 && sDotN < 0.4)
			sDotN = 0.2;
		if(sDotN >= 0.4 && sDotN < 0.5)
			sDotN = 0.4;
		if(sDotN >= 0.5)
			sDotN = 1;
		vec3 diffuse = diffuseLight * sDotN;
		
		// Specular Light
		vec3 specular = vec3(0.0, 0.0, 0.0);
		if(sDotN > 0.0){
			vec3 v = normalize(-camCoords.xyz);
			vec3 r = reflect(-s.xyz, n.xyz);
			specular = specularLight * pow(max(dot(r, v), 0.0), shininess);
		}
		
		lightIntensity = ambient/* + diffuse + specular*/;
		gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);
		
	}
	
	
	
	
	
	