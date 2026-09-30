#include "Skydome.h"

using namespace KamataEngine;

void Skydome::Initialize(Model* model, Camera* camera) {
	worldTransform_.Initialize();
	model_ = model;
	camera_ = camera;
}

void Skydome::Update() {

	worldTransform_.TransferMatrix(); 
}

void Skydome::Draw() {

	model_->Draw(worldTransform_, *camera_);

}