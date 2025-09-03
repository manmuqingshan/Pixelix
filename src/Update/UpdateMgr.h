/* MIT License
 *
 * Copyright (c) 2019 - 2025 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/
/**
 * @brief  Update manager
 * @author Andreas Merkle <web@blue-andi.de>
 * 
 * @addtogroup UPDATE
 *
 * @{
 */

#ifndef UPDATEMGR_H
#define UPDATEMGR_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <SimpleTimer.hpp>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * The update manager handles everything around an on-the-air update.
 */
class UpdateMgr
{
public:

    /**
     * Get update manager instance.
     * 
     * @return Update manager
     */
    static UpdateMgr& getInstance()
    {
        static UpdateMgr instance; /* singleton idiom to force initialization in the first usage. */

        return instance;
    }

    /**
     * Is a restart requested?
     * This will be requested after a successful received new firmware
     * or filesystem.
     * 
     * @return If restart is requested, it will return true otherwise false.
     */
    bool isRestartRequested() const
    {
        return m_isRestartReq;
    }

    /**
     * Handle over-the-air update.
     */
    void process(void);

    /**
     * Request a restart.
     * 
     * @param[in] delay How long the restart shall be delayed in ms.
     */
    void reqRestart(uint32_t delay)
    {
        if (0U == delay)
        {
            m_isRestartReq = true;
        }
        else
        {
            m_timer.start(delay);
        }
    }

private:

    /** Restart requested? */
    bool                m_isRestartReq;

    /** Timer used to delay a restart request. */
    SimpleTimer         m_timer;

    /**
     * Constructs the update manager.
     */
    UpdateMgr();

    /**
     * Destroys the update manager.
     */
    ~UpdateMgr();
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* UPDATEMGR_H */

/** @} */