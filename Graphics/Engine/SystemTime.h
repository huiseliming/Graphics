//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//
// Contains classes needed to time executing code.
//

#pragma once
#include <string>
#include "../Utility/Utility.h"

class SystemTime
{
    friend class GameTimer;
public:

    // Query the performance counter frequency
    static void Initialize( void );

    // Query the current value of the performance counter
    static int64_t GetCurrentTick( void );

    static void BusyLoopSleep( float SleepTime );

    static inline double TicksToSeconds( int64_t TickCount )
    {
        return TickCount * sm_CpuTickDelta;
    }

    static inline double TicksToMillisecs( int64_t TickCount )
    {
        return TickCount * sm_CpuTickDelta * 1000.0;
    }

    static inline double TimeBetweenTicks( int64_t tick1, int64_t tick2 )
    {
        return TicksToSeconds(tick1 - tick2);
    }

private:
    // The amount of time that elapses between ticks of the performance counter
    static double sm_CpuTickDelta;

};

//ÓÎÏ·¼ÆÊ±Æ÷
class GameTimer
{
public:
    GameTimer()
    {
        if (SystemTime::sm_CpuTickDelta == 0.0f)
        {
            SystemTime::Initialize();
            GlobalVariable<GameTimer>::Set(this);
        }
    }
    void Reset()
    {
        m_StartTick = SystemTime::GetCurrentTick();
        m_PreviousvTick = m_StartTick;
        m_CurrentTick = m_StartTick;
        m_StopTickCount = 0;
        m_DeltaTime = 0.f;
        m_Stop =false;
    }

    void Start()
    {
        m_Stop = false;
    }
    
    void Stop()
    {
        m_Stop = true;

    }

    void Tick()
    {
        m_PreviousvTick = m_CurrentTick;
        m_CurrentTick = SystemTime::GetCurrentTick();
        if (m_Stop)
        {
            m_StopTickCount += m_CurrentTick - m_PreviousvTick;
            m_DeltaTime = 0;
        } 
        else 
        {
            m_DeltaTime = (float) SystemTime::TimeBetweenTicks(m_CurrentTick, m_PreviousvTick);
        }
    }

    float GetDeltaTime() { return m_DeltaTime; }
    float GetGameTime() { return (float)SystemTime::TimeBetweenTicks(m_CurrentTick - m_StopTickCount, m_StartTick); }
    float GetGameTimeNoPaused() { return (float)SystemTime::TimeBetweenTicks(m_CurrentTick, m_StartTick); }

private:
    float m_DeltaTime;
    int64_t m_StartTick;
    int64_t m_PausedTick;
    int64_t m_PreviousvTick;
    int64_t m_CurrentTick;
    int64_t m_StopTickCount;
    bool m_Stop;
};







class CpuTimer
{
public:

    CpuTimer()
    {
        m_StartTick = 0ll;
        m_ElapsedTicks = 0ll;
    }

    void Start()
    {
        if (m_StartTick == 0ll)
            m_StartTick = SystemTime::GetCurrentTick();
    }

    void Stop()
    {
        if (m_StartTick != 0ll)
        {
            m_ElapsedTicks += SystemTime::GetCurrentTick() - m_StartTick;
            m_StartTick = 0ll;
        }
    }

    void Reset()
    {
        m_ElapsedTicks = 0ll;
        m_StartTick = 0ll;
    }

    double GetTime() const
    {
        return SystemTime::TicksToSeconds(m_ElapsedTicks);
    }

private:

    int64_t m_StartTick;
    int64_t m_ElapsedTicks;
};









