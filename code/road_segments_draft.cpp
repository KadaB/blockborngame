//NOTE(moritz): Road segment types
road_segment RoadSegments[ROAD_SEGMENT_TYPE_COUNT];
RoadSegments[0].EndRelPX = 0.0f;
RoadSegments[0].Position = 0.0f;
RoadSegments[1].EndRelPX = 20.0f;
RoadSegments[1].Position = MaxDistance;

//NOTE(moritz): Update road segment positions and potentially determine new bottom segment
for(int RoadSegmentIndex = 0;
	RoadSegmentIndex < ROAD_SEGMENT_TYPE_COUNT;
	++RoadSegmentIndex)
{
	RoadSegments[RoadSegmentIndex].Position -= dPlayerP;
	
	if(RoadSegments[RoadSegmentIndex].Position < 0.0f)
	{
		BottomSegment.EndRelPX = RoadSegments[RoadSegmentIndex].EndRelPX;
		
		RoadSegments[RoadSegmentIndex].Position = MaxDistance;
	}
}

//NOTE(moritz): Sort road segments by position
for(int Outer = 0;
	Outer < ROAD_SEGMENT_TYPE_COUNT;
	++Outer)
{
	for(int Inner = 0;
		Inner < (ROAD_SEGMENT_TYPE_COUNT - 1);
		++Inner)
	{
		road_segment *SegmentA = RoadSegments + Inner;
		road_segment *SegmentB = RoadSegments + Inner + 1;
		
		if(SegmentA->Position > SegmentB->Position)
		{
			road_segment Temp = *SegmentA;
			*SegmentA = *SegmentB;
			*SegmentB = Temp;
		}
	}
}

//NOTE(moritz): Determine ddX

//NOTE(moritz): For base segment first
{
	float CurveStartX = 0.0f;
	float CurveEndX   = BottomSegment.EndRelPX;
	
	float DeltaDepthMapP = RoadSegments[0].Position;
	
	BottomSegment.ddX = 2.0f*(CurveEndX - CurveStartX)/((DeltaDepthMapP*(DeltaDepthMapP + 1.0f)));
}

//TODO(moritz): Try this
//float PreviousEndRelPX = BottomSegment.EndRelPX;

//NOTE(moritz): For other segments
for(int RoadSegmentIndex = 0;
	RoadSegmentIndex < (ROAD_SEGMENT_TYPE_COUNT - 1);
	++RoadSegmentIndex)
{
	float CurveStartX = 0.0f;
	float CurveEndX   = RoadSegments[RoadSegmentIndex].EndRelPX;
	
	//TODO(moritz): Which way around?
	float DeltaDepthMapP = RoadSegments[RoadSegmentIndex].Position - RoadSegments[RoadSegmentIndex + 1].Position;
	
	RoadSegments[RoadSegmentIndex].ddX = 2.0f*(CurveEndX - CurveStartX)/((DeltaDepthMapP*(DeltaDepthMapP + 1.0f)));
}