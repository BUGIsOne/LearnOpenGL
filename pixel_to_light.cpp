for(int j=0; j<imageHeight; j++)
{
    for(int i=0; i<imageWidth; i++)
    {
        Ray primRay;
        computePrimRay(i, j, &primRay);
        //shoot prim ray in the scene and search for intersection
        Point pHit;
        Normal nHit;
        float minDist = INFINITY;
        Object object = NULL;
        for(int k=0; k<objects.size(); k++)
        {
            if(Intersect(objects[k], primRay, &pHit, &nHit))
            {
                float distance = Distance(eyePosition, pHit);
                if(distance < minDistance)
                {
                    object = objects[k];
                    minDistance = distance;
                }
            }
        }
    }

    if(object != NULL)
    {
        //compute illumination
        Ray shadowRay;
        shadowRay.direction = lightPosition - pHit;
        bool isShadow = false;
        for(int k=0; k<objects.size(); k++)
        {
            if(Intersect(objects[k], shadowRay))
            {
                isShadow = true;
                break;
            }
        }
    }

    if(!isInShadow)
    {
        pixels[i][j] = object->color * light.brightness;
    }
    else
        pixels[i][j] = 0;

}





