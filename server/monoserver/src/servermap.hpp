/*
 * =====================================================================================
 *
 *       Filename: servermap.hpp
 *        Created: 09/03/2015 03:49:00 AM
 *  Last Modified: 04/22/2016 12:06:41
 *
 *    Description: put all non-atomic function as private
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#pragma once

#include <list>
#include <vector>
#include <cstdint>
#include <forward_list>

#include "mir2xmap.hpp"
#include "asyncobject.hpp"

#include "objectpod.hpp"

class CharObject;
class ServerMap
{
    private:
        ObjectPod *m_ObjectPod;     // to receive external events and heart beat
        Metronome *m_Metronome;     // to generate heart beat for the map

    private:
        void Operate(const MessagePack &, Theron::Address);

    private:
        // some shortcuts for internal use only
        // for public API don't use it
        using ObjectRecord     = std::tuple<uint8_t, uint32_t, uint32_t>;
        using ObjectRecordList = std::list<ObjectRecord>;
        using LockPointer      = std::shared_ptr<std::mutex>;
        template<typename T> using Vec2D = std::vector<std::vector<T>>;

    public:
        ServerMap(uint32_t);
        ~ServerMap() = default;

    public:
        uint32_t ID()
        {
            return m_ID;
        }

    private:
        uint32_t m_ID;

    private:
        Mir2xMap m_Mir2xMap;

    public:
        bool ValidC(int nX, int nY)
        {
            return m_Mir2xMap.ValidC(nX, nY);
        }

        bool ValidP(int nX, int nY)
        {
            return m_Mir2xMap.ValidP(nX, nY);
        }


    private:

        Vec2D<ObjectRecordList>  m_GridObjectRecordListV;
        Vec2D<LockPointer>       m_GridObjectRecordListLockV;

    public:
        bool Load(const char *);
        bool ObjectMove(int, int, CharObject*);
        bool RemoveObject(int, int, uint8_t, uint32_t, uint32_t);

    public:
        bool QueryObject(int, int, const std::function<void(uint8_t, uint32_t, uint32_t)> &);

    private:
        bool GetObjectList(int, int, std::list<ObjectRecord> *, std::function<bool(uint8_t nType)>);

        bool CanSafeWalk(int, int);
        bool CanMove(int, int, int, uint32_t, uint32_t);

    public:
        int W()
        {
            return m_Mir2xMap.W();
        }

        int H()
        {
            return m_Mir2xMap.H();
        }

    public:
        bool DropLocation(int, int, int, int *, int *);

    private:
        // lock/unlock an area *coverd* by (nX, nY, nW, nH)
        // return true if covered cells are all lock/unlocked, fails if nothing done
        bool LockArea(bool bLockIt, int nX, int nY,
                int nW = 1, int nH = 1, int nIgnoreX = -1, int nIgnoreY = -1)
        {
            bool bAffect = false;
            for(int nCX = nX; nCX < nX + nW; ++nCX){
                for(int nCY = nY; nCY < nY + nH; ++nCY){
                    // 1. it's valid
                    // 2. skip the grid we ask to ignore
                    if(m_Mir2xMap.ValidC(nCX, nCY) && (nCX != nIgnoreX) && (nCY != nIgnoreY)){
                        if(bLockIt){
                            m_GridObjectRecordListLockV[nCY][nCX]->lock();
                        }else{
                            m_GridObjectRecordListLockV[nCY][nCX]->unlock();
                        }
                        bAffect = true;
                    }
                }
            }
            return bAffect;
        }

    protected:
        // for region monitors
        typedef struct _RegionMonitorRecord {
            RegionMonitor   *RegionMonitor;
            Theron::Address  PodAddress;
            bool             Need;

            _RegionMonitorRecord()
                : RegionMonitor(nullptr)
                , PodAddress(Theron::Address::Null())
                , Need(false)
            {}
        } RegionMonitorRecord;

        Vec2D<RegionMonitorRecord>  m_RegiongMonitorRecordV2D;

        void CheckRegionMonitorNeed();
        bool CheckRegionMonitorReady()
        {
            for(int nY = 0; nY < H() * SYS_MAPGRIDYP / m_RegiongMonitorResolution; ++nY){
                for(int nX = 0; nX < W() * SYS_MAPGRIDXP / m_RegiongMonitorResolution; ++nX){
                    if(m_RegiongMonitorRecordV2D[nY][nX].Need
                            && m_RegiongMonitorRecordV2D[nY][nX] == Theron::Address::Null()){
                        m_RegionMonitorReady = false;
                        return false;
                    }
                }
            }
            m_RegionMonitorReady = true;
            return true;
        }

        const Theron::Address &RegionMonitorAddressP(int nX, int nY)
        {
            if(!ValidP(nX, nY)){ return m_NullAddress; }

            int nGridX = nX / m_RegiongMonitorResolution;
            int nGridY = nY / m_RegiongMonitorResolution;
            return m_RegiongMonitorRecordV2D[nGridY][nGridX].PodAddress;
        }

        bool RegionMonitorReady()
        {
            return m_RegionMonitorReady;
        }
};
