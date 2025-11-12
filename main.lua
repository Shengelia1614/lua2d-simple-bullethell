local PlayingState = require('states.playing')
local GameOverState = require('states.gameover')

-- game logic is as follows
-- you there is an enemy moving around the screen bouncing off walls and shooting bullets at the player
-- the player can move around and deploy a barrier that bounces the enemy bullets away if it collides with it
-- since player is faster than the enemy if you deplay barier and run into him you may trigger colision even tho there is a barrier
-- there is no winning  your gaol is to get highest survival time,
-- at the start bullets can only bounce 3 times
-- every 30 seconds the max bounces increase by 1

local states = {
    playing = PlayingState,
    gameover = GameOverState
}

local currentState = nil
local currentStateName = nil

local function changeState(stateName)
    if currentState and currentState.exit then
        currentState:exit()
    end

    currentStateName = stateName
    currentState = states[stateName]

    if currentState and currentState.enter then
        currentState:enter()
    end
end

function love.load()
    love.window.setTitle('Bullet Hell')
    love.window.setMode(800, 600)
    math.randomseed(os.time())
    changeState('playing')
end

function love.update(dt)
    if currentState and currentState.update then
        local nextState = currentState:update(dt)
        if nextState then
            changeState(nextState)
        end
    end
end

function love.draw()
    love.graphics.clear(0.1, 0.1, 0.15)

    if currentState and currentState.draw then
        currentState:draw()
    end
end
