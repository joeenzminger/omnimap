Projector = ClassProjector:newdefaults()

Projector.Point_ProjectorPostion = Vector(0,0,0)
Projector.Vec_ProjectorLookVec   = Vector(0,-1,0)
Projector.Vec_ProjectorUpVec     = Vector(0,0,-1)

func_OpticalOffsetAxis = LensCrop_VDome

AudiencePos			    = Vector(0.00, 0.0 ,0.00) 

BaseViewDirection = Vector(0,0,-1)
BaseHeadsUp       = Vector(0,1,0)

createadome      	= false




DomeCenter =Vector(0,-1,0)
DomeTesselation = 100
cylinder_radius =1
cylinder_height =1
vec_up     =Vector(0,1,0)
vec_forward=Vector(0,0,-1)
theta_min = -3.14/2.0
theta_max = 3.14/2.0
ScreenRed = 1
ScreenGreen =1
ScreenBlue =1
createacylinder = true





leftChannel   = ClassRenderChannel:new("Left")
rightChannel  = ClassRenderChannel:new("Right")

-- if we want to maximize our channel res...
leftChannel:SetCameraInfoAsymm(45, 45, 55, 35, .1,1000)
rightChannel:SetCameraInfoAsymm(45, 45, 55, 35,.1,1000)




