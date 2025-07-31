struct LightHitInfo
{
    //  a bit field containing the info of which lights hit this point
    //  the n-th bit set to 1 == the n-th light hits this point
    uint lights;
};
